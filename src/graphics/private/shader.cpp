#include "graphics/shader.h"
#include "graphics/private/shader_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/manager.h"

#include <algorithm>

namespace GPU
{
	bool operator!=(const BindingSRV& a, const BindingSRV& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingUAV& a, const BindingUAV& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingBuffer& a, const BindingBuffer& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingSampler& a, const BindingSampler& b) { return a.resource_ != a.resource_; }
}

namespace Graphics
{
	class ShaderFactory : public Resource::IFactory
	{
	public:
		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Shader::GetTypeUUID());
			*outResource = new Shader();
			return true;
		}

		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Shader::GetTypeUUID());
			auto* shader = reinterpret_cast<Shader*>(*inResource);
			delete shader;
			*inResource = nullptr;
			return true;
		}

		bool LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
		    const char* name, Core::File& inFile) override
		{
			Shader* shader = *reinterpret_cast<Shader**>(inResource);
			DBG_ASSERT(shader);

			const bool isReload = shader->IsReady();
			DBG_ASSERT(shader->impl_ == nullptr || isReload);

			Graphics::ShaderHeader header;
			i32 readBytes = 0;

			// Read in desc.
			readBytes = sizeof(header);
			if(inFile.Read(&header, readBytes) != readBytes)
			{
				return false;
			}

			// Check magic.
			if(header.magic_ != Graphics::ShaderHeader::MAGIC)
				return false;

			// Check version.
			if(header.majorVersion_ != Graphics::ShaderHeader::MAJOR_VERSION)
				return false;

			if(header.minorVersion_ != Graphics::ShaderHeader::MINOR_VERSION)
				DBG_LOG("Minor version differs from expected. Can still load successfully.");

			// Creating shader impl.
			auto* impl = new ShaderImpl();
			impl->name_ = name;
			impl->header_ = header;

			auto OnFailure = [impl]() { delete impl; };

			impl->bindingHeaders_.resize(header.numCBuffers_ + header.numSRVs_ + header.numUAVs_ + header.numSamplers_);
			readBytes = impl->bindingHeaders_.size() * sizeof(ShaderBindingHeader);
			if(inFile.Read(impl->bindingHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			impl->bytecodeHeaders_.resize(header.numShaders_);
			readBytes = impl->bytecodeHeaders_.size() * sizeof(ShaderBytecodeHeader);
			if(inFile.Read(impl->bytecodeHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			i32 numBindingMappings = 0;
			i32 bytecodeSize = 0;
			for(const auto& bytecodeHeader : impl->bytecodeHeaders_)
			{
				numBindingMappings += bytecodeHeader.numCBuffers_ + bytecodeHeader.numSamplers_ +
				                      bytecodeHeader.numSRVs_ + bytecodeHeader.numUAVs_;
				bytecodeSize = Core::Max(bytecodeSize, bytecodeHeader.offset_ + bytecodeHeader.numBytes_);
			}

			impl->bindingMappings_.resize(numBindingMappings);
			readBytes = impl->bindingMappings_.size() * sizeof(ShaderBindingMapping);
			if(inFile.Read(impl->bindingMappings_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			impl->techniqueHeaders_.resize(header.numTechniques_);
			readBytes = impl->techniqueHeaders_.size() * sizeof(ShaderTechniqueHeader);
			if(inFile.Read(impl->techniqueHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			impl->samplerStateHeaders_.resize(header.numSamplerStates_);
			readBytes = impl->samplerStateHeaders_.size() * sizeof(ShaderSamplerStateHeader);
			if(inFile.Read(impl->samplerStateHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			impl->bytecode_.resize(bytecodeSize);
			readBytes = impl->bytecode_.size();
			if(inFile.Read(impl->bytecode_.data(), readBytes) != readBytes)
			{
				OnFailure();
				return false;
			}

			// Create all the shaders & sampler states.
			GPU::Handle handle;
			if(GPU::Manager::IsInitialized())
			{
				const ShaderBindingMapping* mapping = impl->bindingMappings_.data();
				impl->shaders_.reserve(impl->shaders_.size());
				impl->shaderBindingMappings_.reserve(impl->shaders_.size());
				for(const auto& bytecode : impl->bytecodeHeaders_)
				{
					GPU::ShaderDesc desc;
					desc.data_ = &impl->bytecode_[bytecode.offset_];
					desc.dataSize_ = bytecode.numBytes_;
					desc.type_ = bytecode.type_;
					handle = GPU::Manager::CreateShader(desc, name);
					if(!handle)
					{
						OnFailure();
						return false;
					}

					impl->shaders_.push_back(handle);

					impl->shaderBindingMappings_.push_back(mapping);

					mapping += (bytecode.numCBuffers_ + bytecode.numSamplers_ + bytecode.numSRVs_ + bytecode.numUAVs_);
				}

				impl->samplerStates_.reserve(impl->samplerStateHeaders_.size());
				for(const auto& samplerStateHeader : impl->samplerStateHeaders_)
				{
					handle = GPU::Manager::CreateSamplerState(samplerStateHeader.state_, samplerStateHeader.name_);
					if(!handle)
					{
						OnFailure();
						return false;
					}
					impl->samplerStates_.push_back(handle);
				}

				// Bytecode no longer needed once created.
				impl->bytecode_.clear();
			}

			if(isReload)
			{
				auto reloadLock = Resource::Manager::TakeReloadLock();

				// Setup technique descs, hashes, and empty pipeline states.
				std::swap(impl->techniqueDescHashes_, shader->impl_->techniqueDescHashes_);
				std::swap(impl->techniqueDescs_, shader->impl_->techniqueDescs_);
				impl->pipelineStates_.resize(impl->techniqueDescs_.size());

				// Swap techniques over.
				std::swap(impl->techniques_, shader->impl_->techniques_);

				// Setup techniques again in their new impl.
				for(i32 idx = 0; idx < impl->techniques_.size(); ++idx)
				{
					auto* techImpl = impl->techniques_[idx];
					techImpl->shader_ = impl;
					impl->SetupTechnique(techImpl);
				}

				std::swap(shader->impl_, impl);
				delete impl;
			}
			else
			{
				std::swap(shader->impl_, impl);
				DBG_ASSERT(impl == nullptr);
			}

			return true;
		}
	};

	DEFINE_RESOURCE(Shader);

	Shader::Shader()
	{ //
	}

	Shader::~Shader()
	{ //
		DBG_ASSERT(impl_);
		delete impl_;
	}

	i32 Shader::GetBindingIndex(const char* name) const { return impl_->GetBindingIndex(name); }

	ShaderTechnique Shader::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		ShaderTechnique tech;
		tech.impl_ = impl_->CreateTechnique(name, desc);
		return tech;
	}


	ShaderTechnique::ShaderTechnique() {}

	ShaderTechnique::~ShaderTechnique()
	{
		if(impl_)
		{
			Job::ScopedWriteLock lock(impl_->shader_->rwLock_);

			if(GPU::Manager::IsInitialized())
				if(impl_->bsHandle_)
					GPU::Manager::DestroyResource(impl_->bsHandle_);

			auto it = std::find(impl_->shader_->techniques_.begin(), impl_->shader_->techniques_.end(), impl_);
			DBG_ASSERT(it != impl_->shader_->techniques_.end());
			impl_->shader_->techniques_.erase(it);

			delete impl_;
			impl_ = nullptr;
		}
	}

	ShaderTechnique::ShaderTechnique(ShaderTechnique&& other)
	    : impl_(nullptr)
	{
		std::swap(impl_, other.impl_);
	}

	ShaderTechnique& ShaderTechnique::operator=(ShaderTechnique&& other)
	{
		std::swap(impl_, other.impl_);
		return *this;
	}

	void ShaderTechnique::SetSampler(i32 idx, GPU::Handle res)
	{
		idx -= impl_->samplerOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->samplers_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		GPU::BindingSampler binding;
		binding.resource_ = res;
		if(binding != impl_->samplers_[idx])
		{
			impl_->samplers_[idx] = binding;
			impl_->bsDirty_ = true;
		}
	}

	bool ShaderTechnique::Set(i32 idx, const GPU::BindingCBV& binding)
	{
		idx -= impl_->cbvOffset_;
		if(idx >= 0 && idx < impl_->cbvs_.size())
		{
			if(binding != impl_->cbvs_[idx])
			{
				impl_->cbvs_[idx] = binding;
				impl_->bsDirty_ = true;
			}
			return true;
		}
		return false;
	}

	bool ShaderTechnique::Set(i32 idx, const GPU::BindingSRV& binding)
	{
		idx -= impl_->srvOffset_;
		if(idx >= 0 && idx < impl_->srvs_.size())
		{
			if(binding != impl_->srvs_[idx])
			{
				impl_->srvs_[idx] = binding;
				impl_->bsDirty_ = true;
			}
			return true;
		}
		return false;
	}

	bool ShaderTechnique::Set(i32 idx, const GPU::BindingUAV& binding)
	{
		idx -= impl_->uavOffset_;
		if(idx >= 0 && idx < impl_->uavs_.size())
		{
			if(binding != impl_->uavs_[idx])
			{
				impl_->uavs_[idx] = binding;
				impl_->bsDirty_ = true;
			}
			return true;
		}
		return false;
	}

	bool ShaderTechnique::Set(const char* name, const GPU::BindingCBV& binding)
	{
		return Set(impl_->shader_->GetBindingIndex(name), binding);
	}

	bool ShaderTechnique::Set(const char* name, const GPU::BindingSRV& binding)
	{
		return Set(impl_->shader_->GetBindingIndex(name), binding);
	}

	bool ShaderTechnique::Set(const char* name, const GPU::BindingUAV& binding)
	{
		return Set(impl_->shader_->GetBindingIndex(name), binding);
	}

	GPU::Handle ShaderTechnique::GetBinding()
	{
		if(impl_ && impl_->bsDirty_)
		{
			if(impl_->bsHandle_)
			{
				GPU::Manager::DestroyResource(impl_->bsHandle_);
				impl_->bsHandle_ = GPU::Handle();
			}

			if(impl_->IsValid())
			{
				impl_->bs_.numCBVs_ = 0;
				impl_->bs_.numSamplers_ = 0;
				impl_->bs_.numSRVs_ = 0;
				impl_->bs_.numUAVs_ = 0;

				// Setup binding set with offsets that come from shaders.
				const auto SetupCBV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
					for(i32 idx = 0; idx < numMappings; ++idx)
					{
						const i32 dst = mapping[idx].dstSlot_;
						const i32 binding = mapping[idx].binding_;
						impl_->bs_.cbvs_[dst] = impl_->cbvs_[binding - impl_->cbvOffset_];
						impl_->bs_.numCBVs_ = Core::Max(impl_->bs_.numCBVs_, dst + 1);
					}
					return mapping + numMappings;
				};

				const auto SetupSampler = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
					for(i32 idx = 0; idx < numMappings; ++idx)
					{
						const i32 dst = mapping[idx].dstSlot_;
						const i32 binding = mapping[idx].binding_;
						impl_->bs_.samplers_[dst] = impl_->samplers_[binding - impl_->samplerOffset_];
						impl_->bs_.numSamplers_ = Core::Max(impl_->bs_.numSamplers_, dst + 1);
					}
					return mapping + numMappings;
				};

				const auto SetupSRV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
					for(i32 idx = 0; idx < numMappings; ++idx)
					{
						const i32 dst = mapping[idx].dstSlot_;
						const i32 binding = mapping[idx].binding_;
						impl_->bs_.srvs_[dst] = impl_->srvs_[binding - impl_->srvOffset_];
						impl_->bs_.numSRVs_ = Core::Max(impl_->bs_.numSRVs_, dst + 1);
					}
					return mapping + numMappings;
				};

				const auto SetupUAV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
					for(i32 idx = 0; idx < numMappings; ++idx)
					{
						const i32 dst = mapping[idx].dstSlot_;
						const i32 binding = mapping[idx].binding_;
						impl_->bs_.uavs_[dst] = impl_->uavs_[binding - impl_->uavOffset_];
						impl_->bs_.numUAVs_ = Core::Max(impl_->bs_.numUAVs_, dst + 1);
					}
					return mapping + numMappings;
				};

				const auto SetupBindings = [&](i32 shaderIdx) {
					if(shaderIdx < 0)
						return;

					const auto& bytecode = impl_->shader_->bytecodeHeaders_[shaderIdx];
					const auto* mappings = impl_->shader_->shaderBindingMappings_[shaderIdx];

					mappings = SetupCBV(mappings, bytecode.numCBuffers_);
					mappings = SetupSRV(mappings, bytecode.numSRVs_);
					mappings = SetupUAV(mappings, bytecode.numUAVs_);
					mappings = SetupSampler(mappings, bytecode.numSamplers_);
				};

				SetupBindings(impl_->header_.vs_);
				SetupBindings(impl_->header_.gs_);
				SetupBindings(impl_->header_.hs_);
				SetupBindings(impl_->header_.ds_);
				SetupBindings(impl_->header_.ps_);
				SetupBindings(impl_->header_.cs_);

				Core::Array<char, 128> debugName = {};
				sprintf_s(debugName.data(), debugName.size(), "%s/%s_binding", impl_->shader_->name_.c_str(),
				    impl_->header_.name_);
				impl_->bsHandle_ = GPU::Manager::CreatePipelineBindingSet(impl_->bs_, debugName.data());
				DBG_ASSERT(impl_->bsHandle_);
				DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(impl_->bsHandle_));
				impl_->bsDirty_ = false;
			}
		}
		return impl_ ? impl_->bsHandle_ : GPU::Handle();
	}

	ShaderTechnique::operator bool() const { return !!impl_ && impl_->IsValid(); }

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetVertexElement(i32 idx, const GPU::VertexElement& element)
	{
		numVertexElements_ = Core::Max(numVertexElements_, idx + 1);
		vertexElements_[idx] = element;
		return *this;
	}

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetVertexElements(Core::ArrayView<GPU::VertexElement> elements)
	{
		numVertexElements_ = 0;
		for(const auto& element : elements)
			vertexElements_[numVertexElements_++] = element;
		return *this;
	}

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetTopology(GPU::TopologyType topology)
	{
		topology_ = topology;
		return *this;
	}

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetRTVFormat(i32 idx, GPU::Format format)
	{
		numRTs_ = Core::Max(numRTs_, idx + 1);
		rtvFormats_[idx] = format;
		return *this;
	}

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetDSVFormat(GPU::Format format)
	{
		dsvFormat_ = format;
		return *this;
	}

	ShaderTechniqueDesc& ShaderTechniqueDesc::SetFrameBindingSet(const GPU::FrameBindingSetDesc& desc)
	{
		numRTs_ = 0;
		i32 idx = 0;
		for(const auto& rtv : desc.rtvs_)
		{
			if(rtv.format_ != GPU::Format::INVALID)
				rtvFormats_[numRTs_++] = rtv.format_;
			else
				break;
			++idx;
		}
		dsvFormat_ = desc.dsv_.format_;
		return *this;
	}

	bool operator!=(const GPU::VertexElement& a, const GPU::VertexElement& b)
	{
		if(a.streamIdx_ != b.streamIdx_ || a.offset_ != b.offset_ || a.format_ != b.format_ || a.usage_ != b.usage_ ||
		    a.usageIdx_ != b.usageIdx_)
			return true;
		return false;
	}

	bool operator==(const ShaderTechniqueDesc& a, const ShaderTechniqueDesc& b)
	{
		if(a.numVertexElements_ != b.numVertexElements_)
			return false;
		for(i32 idx = 0; idx < a.numVertexElements_; ++idx)
			if(a.vertexElements_[idx] != b.vertexElements_[idx])
				return false;
		if(a.topology_ != b.topology_)
			return false;
		if(a.numRTs_ != b.numRTs_)
			return false;
		for(i32 idx = 0; idx < a.numRTs_; ++idx)
			if(a.rtvFormats_[idx] != b.rtvFormats_[idx])
				return false;
		if(a.dsvFormat_ != b.dsvFormat_)
			return false;

		return true;
	}

	ShaderImpl::ShaderImpl() {}

	ShaderImpl::~ShaderImpl()
	{
		DBG_ASSERT_MSG(techniques_.size() == 0, "Techniques still reference this shader.");

		if(GPU::Manager::IsInitialized())
		{
			for(auto ps : pipelineStates_)
				GPU::Manager::DestroyResource(ps);
			for(auto s : shaders_)
				GPU::Manager::DestroyResource(s);
			for(auto s : samplerStates_)
				GPU::Manager::DestroyResource(s);
		}
	}

	i32 ShaderImpl::GetBindingIndex(const char* name) const
	{
		for(i32 idx = 0; idx < bindingHeaders_.size(); ++idx)
		{
			const auto& binding = bindingHeaders_[idx];
			if(strcmp(binding.name_, name) == 0)
				return idx;
		}
		return -1;
	}

	const char* ShaderImpl::GetBindingName(i32 idx) const { return bindingHeaders_[idx].name_; }

	ShaderTechniqueImpl* ShaderImpl::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		Job::ScopedWriteLock lock(rwLock_);

		// See if there is a matching name + descriptor, if not, add it.
		i32 foundIdx = -1;
		u32 hash = Core::HashCRC32(0, &desc, sizeof(desc));
		hash = Core::Hash(hash, name);
		for(i32 idx = 0; idx < techniqueDescHashes_.size(); ++idx)
		{
			if(techniqueDescHashes_[idx] == hash)
			{
				DBG_ASSERT_MSG(techniqueDescs_[idx] == desc, "Technique hash collision!");
				foundIdx = idx;
			}
		}

		// None found, push to end of list.
		if(foundIdx == -1)
		{
			techniqueDescHashes_.push_back(hash);
			techniqueDescs_.push_back(desc);
			pipelineStates_.resize(techniqueDescs_.size());
			foundIdx = techniqueDescs_.size() - 1;
		}

		auto* impl = new ShaderTechniqueImpl();
		impl->shader_ = this;
		strcpy_s(impl->header_.name_, sizeof(impl->header_.name_), name);
		impl->descIdx_ = foundIdx;

		techniques_.push_back(impl);

		// Setup newly created technique immediately.
		SetupTechnique(impl);

		return impl;
	}

	bool ShaderImpl::SetupTechnique(ShaderTechniqueImpl* impl)
	{
		DBG_ASSERT(impl);
		DBG_ASSERT(impl->descIdx_ != -1);
		DBG_ASSERT(impl->descIdx_ < pipelineStates_.size());

		// Find valid technique header.
		const ShaderTechniqueHeader* techHeader = nullptr;
		for(const auto& it : techniqueHeaders_)
		{
			if(strcmp(it.name_, impl->header_.name_) == 0)
			{
				techHeader = &it;
				break;
			}
		}

		if(techHeader == nullptr)
		{
			DBG_LOG("SetupTechnique: Shader \'%s\' is missing technique \'%s\'\n", name_.c_str(), impl->header_.name_);
			impl->Invalidate();
			return false;
		}

		// Create pipeline state for technique if there is none.
		GPU::Handle psHandle = pipelineStates_[impl->descIdx_];
		if(!psHandle && GPU::Manager::IsInitialized())
		{
			const auto& desc = techniqueDescs_[impl->descIdx_];
			Core::Array<char, 128> debugName = {};
			sprintf_s(debugName.data(), debugName.size(), "%s/%s", name_.c_str(), impl->header_.name_);
			DBG_ASSERT(techHeader->vs_ != -1 || techHeader->cs_ != -1);

			if(techHeader->cs_ != -1)
			{
				GPU::ComputePipelineStateDesc psDesc;
				psDesc.shader_ = shaders_[techHeader->cs_];
				psHandle = GPU::Manager::CreateComputePipelineState(psDesc, debugName.data());
			}
			else
			{
				GPU::GraphicsPipelineStateDesc psDesc;
				psDesc.shaders_[(i32)GPU::ShaderType::VS] =
				    techHeader->vs_ != -1 ? shaders_[techHeader->vs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::GS] =
				    techHeader->gs_ != -1 ? shaders_[techHeader->gs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::HS] =
				    techHeader->hs_ != -1 ? shaders_[techHeader->hs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::DS] =
				    techHeader->ds_ != -1 ? shaders_[techHeader->ds_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::PS] =
				    techHeader->ps_ != -1 ? shaders_[techHeader->ps_] : GPU::Handle();
				psDesc.renderState_ = techHeader->rs_;
				psDesc.numVertexElements_ = desc.numVertexElements_;
				memcpy(&psDesc.vertexElements_[0], desc.vertexElements_.data(), sizeof(psDesc.vertexElements_));
				psDesc.topology_ = desc.topology_;
				psDesc.numRTs_ = desc.numRTs_;
				memcpy(&psDesc.rtvFormats_[0], desc.rtvFormats_.data(), sizeof(psDesc.rtvFormats_));
				psDesc.dsvFormat_ = desc.dsvFormat_;
				psHandle = GPU::Manager::CreateGraphicsPipelineState(psDesc, debugName.data());
			}
			pipelineStates_[impl->descIdx_] = psHandle;
		}

		if(!psHandle)
		{
			DBG_LOG("SetupTechnique: Failed to create pipeline state for technique \'%s\' in shader \'%s\'\n",
			    impl->header_.name_, name_.c_str());
			impl->Invalidate();
			return false;
		}

		impl->shader_ = this;
		impl->header_ = *techHeader;
		impl->cbvs_.resize(header_.numCBuffers_);
		impl->srvs_.resize(header_.numSRVs_);
		impl->uavs_.resize(header_.numUAVs_);
		impl->samplers_.resize(header_.numSamplers_);
		impl->bs_.pipelineState_ = psHandle;
		impl->bsDirty_ = true;

		// Calculate offset into vectors for setting.
		i32 offset = 0;
		impl->cbvOffset_ = offset;
		offset += impl->cbvs_.size();

		impl->srvOffset_ = offset;
		offset += impl->srvs_.size();

		impl->uavOffset_ = offset;
		offset += impl->uavs_.size();

		impl->samplerOffset_ = offset;
		offset += impl->samplers_.size();

		// Set samplers.
		for(i32 idx = 0; idx < impl->samplers_.size(); ++idx)
		{
			const char* name = GetBindingName(idx + impl->samplerOffset_);

			for(i32 headerIdx = 0; headerIdx < samplerStateHeaders_.size(); ++headerIdx)
			{
				const auto& header = samplerStateHeaders_[headerIdx];
				if(strcmp(name, header.name_) == 0)
				{
					GPU::BindingSampler binding;
					binding.resource_ = samplerStates_[headerIdx];
					impl->samplers_[idx] = binding;
					break;
				}
			}
		}

		return true;
	}

} // namespace Graphics
