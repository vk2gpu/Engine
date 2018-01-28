#include "graphics/shader.h"
#include "graphics/private/shader_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/enum.h"
#include "gpu/manager.h"
#include "serialization/serializer.h"

#include <algorithm>

namespace GPU
{
	bool operator!=(const SamplerState& a, const SamplerState& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingCBV& a, const BindingCBV& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingSRV& a, const BindingSRV& b) { return memcmp(&a, &b, sizeof(a)) != 0; }

	bool operator!=(const BindingUAV& a, const BindingUAV& b) { return memcmp(&a, &b, sizeof(a)) != 0; }
}

namespace Graphics
{
	bool ShaderHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_MEMBER(magic_);
		SERIALIZE_MEMBER(majorVersion_);
		SERIALIZE_MEMBER(minorVersion_);
		SERIALIZE_MEMBER(numShaders_);
		SERIALIZE_MEMBER(numTechniques_);
		SERIALIZE_MEMBER(numSamplerStates_);
		SERIALIZE_MEMBER(numBindingSets_);
		return true;
	}

	bool ShaderBindingSetHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_STRING_MEMBER(name_);
		SERIALIZE_MEMBER(isShared_);
		SERIALIZE_MEMBER(frequency_);
		SERIALIZE_MEMBER(numCBVs_);
		SERIALIZE_MEMBER(numSRVs_);
		SERIALIZE_MEMBER(numUAVs_);
		SERIALIZE_MEMBER(numSamplers_);
		return true;
	}

	bool ShaderBindingHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_STRING_MEMBER(name_);
		serializer.Serialize("handle_", (u32&)handle_);
		return true;
	}

	bool ShaderBytecodeHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_MEMBER(type_);
		SERIALIZE_MEMBER(offset_);
		SERIALIZE_MEMBER(numBytes_);
		return true;
	}

	bool ShaderTechniqueHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_STRING_MEMBER(name_);
		SERIALIZE_MEMBER(vs_);
		SERIALIZE_MEMBER(gs_);
		SERIALIZE_MEMBER(hs_);
		SERIALIZE_MEMBER(ds_);
		SERIALIZE_MEMBER(ps_);
		SERIALIZE_MEMBER(cs_);
		SERIALIZE_BINARY_MEMBER(rs_);
		return true;
	}

	bool ShaderSamplerStateHeader::Serialize(Serialization::Serializer& serializer)
	{
		SERIALIZE_STRING_MEMBER(name_);
		SERIALIZE_BINARY_MEMBER(state_);
		return true;
	}

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

			ShaderImpl* impl = nullptr;
			Graphics::ShaderHeader header;
			i32 readBytes = 0;

			auto OnFailure = [impl](const char* error) {
				DBG_LOG("ShaderFactory: Failed to load. Error: %s\n", error);
				delete impl;
			};

			// Read in desc.
			readBytes = sizeof(header);
			if(inFile.Read(&header, readBytes) != readBytes)
			{
				OnFailure("Failed to read shader header");
				return false;
			}

			// Check magic.
			if(header.magic_ != Graphics::ShaderHeader::MAGIC)
				return false;

			// Check version.
			if(header.majorVersion_ != Graphics::ShaderHeader::MAJOR_VERSION)
			{
				OnFailure("Shader major version mismatch.");
				return false;
			}

			if(header.minorVersion_ != Graphics::ShaderHeader::MINOR_VERSION)
				DBG_LOG("Minor version differs from expected. Can still load successfully.");

			// Creating shader impl.
			impl = new ShaderImpl();
			impl->name_ = name;
			impl->header_ = header;

			impl->bindingSetHeaders_.resize(header.numBindingSets_);
			readBytes = impl->bindingSetHeaders_.size() * sizeof(ShaderBindingSetHeader);
			if(inFile.Read(impl->bindingSetHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read binding set headers.");
				return false;
			}

			i32 numBindings = 0;
			for(const auto& bindingSet : impl->bindingSetHeaders_)
			{
				numBindings += bindingSet.numCBVs_;
				numBindings += bindingSet.numSRVs_;
				numBindings += bindingSet.numUAVs_;
				numBindings += bindingSet.numSamplers_;
			}

			impl->bindingHeaders_.resize(numBindings);
			readBytes = impl->bindingHeaders_.size() * sizeof(ShaderBindingHeader);
			if(inFile.Read(impl->bindingHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read binding headers.");
				return false;
			}

			impl->bytecodeHeaders_.resize(header.numShaders_);
			readBytes = impl->bytecodeHeaders_.size() * sizeof(ShaderBytecodeHeader);
			if(inFile.Read(impl->bytecodeHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read bytecode headers.");
				return false;
			}

			i32 bytecodeSize = 0;
			for(const auto& bytecodeHeader : impl->bytecodeHeaders_)
			{
				bytecodeSize = Core::Max(bytecodeSize, bytecodeHeader.offset_ + bytecodeHeader.numBytes_);
			}

			impl->techniqueHeaders_.resize(header.numTechniques_);
			readBytes = impl->techniqueHeaders_.size() * sizeof(ShaderTechniqueHeader);
			if(inFile.Read(impl->techniqueHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read technique headers.");
				return false;
			}

			impl->samplerStateHeaders_.resize(header.numSamplerStates_);
			readBytes = impl->samplerStateHeaders_.size() * sizeof(ShaderSamplerStateHeader);
			if(inFile.Read(impl->samplerStateHeaders_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read sampler state headers.");
				return false;
			}

			impl->bytecode_.resize(bytecodeSize);
			readBytes = impl->bytecode_.size();
			if(inFile.Read(impl->bytecode_.data(), readBytes) != readBytes)
			{
				OnFailure("Unable to read bytecode.");
				return false;
			}

			// Create all the shaders & sampler states.
			GPU::Handle handle;
			if(GPU::Manager::IsInitialized())
			{
				impl->shaders_.reserve(impl->shaders_.size());
				i32 shaderIdx = 0;
				for(const auto& bytecode : impl->bytecodeHeaders_)
				{
					GPU::ShaderDesc desc;
					desc.data_ = &impl->bytecode_[bytecode.offset_];
					desc.dataSize_ = bytecode.numBytes_;
					desc.type_ = bytecode.type_;
					handle = GPU::Manager::CreateShader(desc, "%s/shader_%d", name, shaderIdx++);
					if(!handle)
					{
						OnFailure("Unable to create shader.");
						return false;
					}

					impl->shaders_.push_back(handle);
				}

				impl->samplerStates_.reserve(impl->samplerStateHeaders_.size());

				// Bytecode no longer needed once created.
				impl->bytecode_.clear();
			}

			// Add binding sets to the factory.
			if(auto writeLock = Core::ScopedWriteLock(rwLock_))
			{
				i32 handleOffset = 0;
				for(const auto& bindingSetHeader : impl->bindingSetHeaders_)
				{
					const i32 numHandles = bindingSetHeader.numCBVs_ + bindingSetHeader.numSRVs_ +
					                       bindingSetHeader.numUAVs_ + bindingSetHeader.numSamplers_;

					auto it = std::find_if(bindingSetHeaders_.begin(), bindingSetHeaders_.end(),
					    [&bindingSetHeader](const ShaderBindingSetHeader& other) {
						    return memcmp(&bindingSetHeader, &other, sizeof(ShaderBindingSetHeader)) == 0;
						});

					if(it == bindingSetHeaders_.end())
					{
						bindingSetHeaders_.push_back(bindingSetHeader);

						const auto* handleBegin = impl->bindingHeaders_.data() + handleOffset;
						const auto* handleEnd = handleBegin + numHandles;

						BindingSetHandles handles;
						handles.headers_.insert(handleBegin, handleEnd);
						bindingSetHandles_.emplace_back(std::move(handles));
					}

					handleOffset += numHandles;
				}
			}

			// Remap binding set indices.
			if(auto readLock = Core::ScopedReadLock(rwLock_))
			{
				for(auto& techHeaders : impl->techniqueHeaders_)
				{
					for(auto& bindingSlot : techHeaders.bindingSlots_)
					{
						if(bindingSlot.idx_ == -1)
							break;

						const auto& bindingSetHeader = impl->bindingSetHeaders_[bindingSlot.idx_];

						bindingSlot.idx_ = FindBindingSetIdx(bindingSetHeader);
						DBG_ASSERT(bindingSlot.idx_ >= 0);
					}
				}
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

		bool SerializeSettings(Serialization::Serializer& ser) override { return true; }

		i32 FindBindingSetIdx(const char* name)
		{
			for(i32 idx = 0; idx < bindingSetHeaders_.size(); ++idx)
			{
				const auto& bindingSetHeader = bindingSetHeaders_[idx];
				if(strcmp(name, bindingSetHeader.name_) == 0)
				{
					return idx;
				}
			}
			return -1;
		}

		i32 FindBindingSetIdx(const ShaderBindingSetHeader& header)
		{
			for(i32 idx = 0; idx < bindingSetHeaders_.size(); ++idx)
			{
				const auto& bindingSetHeader = bindingSetHeaders_[idx];
				if(memcmp(&header, &bindingSetHeader, sizeof(ShaderBindingSetHeader)) == 0)
				{
					return idx;
				}
			}
			return -1;
		}

		Core::RWLock rwLock_;
		Core::Vector<ShaderBindingSetHeader> bindingSetHeaders_;

		struct BindingSetHandles
		{
			Core::Vector<ShaderBindingHeader> headers_;
		};

		Core::Vector<BindingSetHandles> bindingSetHandles_;
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

	ShaderTechnique Shader::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		ShaderTechnique tech;
		tech.impl_ = impl_->CreateTechnique(name, desc);
		return tech;
	}

	ShaderBindingSet Shader::CreateBindingSet(const char* name)
	{
		ShaderBindingSet bindingSet;
		bindingSet.impl_ = impl_->CreateBindingSet(name);
#if !defined(_RELEASE)
		if(bindingSet.impl_)
			bindingSet.name_ = bindingSet.impl_->header_.name_;
#endif
		return bindingSet;
	}

	ShaderBindingSet Shader::CreateSharedBindingSet(const char* name)
	{
		auto CreateBindingSetInternal = [](const char* name) {
			ShaderBindingSetImpl* bindingSet = nullptr;
			auto* factory = Shader::GetFactory();
			if(auto readLock = Core::ScopedReadLock(factory->rwLock_))
			{
				i32 idx = factory->FindBindingSetIdx(name);
				if(idx >= 0)
				{
					const auto& bindingSetHeader = factory->bindingSetHeaders_[idx];
					if(strcmp(name, bindingSetHeader.name_) == 0)
					{
						bindingSet = new ShaderBindingSetImpl();
						bindingSet->header_ = bindingSetHeader;
						bindingSet->idx_ = idx;

						if(GPU::Manager::IsInitialized())
						{
							GPU::PipelineBindingSetDesc desc;
							desc.shaderVisible_ = false;
							desc.numCBVs_ = bindingSetHeader.numCBVs_;
							desc.numSRVs_ = bindingSetHeader.numSRVs_;
							desc.numUAVs_ = bindingSetHeader.numUAVs_;
							desc.numSamplers_ = bindingSetHeader.numSamplers_;

							bindingSet->pbs_ =
							    GPU::Manager::CreatePipelineBindingSet(desc, "SHARED/%s", bindingSet->header_.name_);
						}

						bindingSet->cbvs_.resize(bindingSetHeader.numCBVs_);
						bindingSet->srvs_.resize(bindingSetHeader.numSRVs_);
						bindingSet->uavs_.resize(bindingSetHeader.numUAVs_);
						bindingSet->samplers_.resize(bindingSetHeader.numSamplers_);
					}
				}
			}
			return bindingSet;
		};

		ShaderBindingSet bindingSet;
		bindingSet.impl_ = CreateBindingSetInternal(name);

#if !defined(_RELEASE)
		if(bindingSet.impl_)
			bindingSet.name_ = bindingSet.impl_->header_.name_;
#endif
		return bindingSet;
	}

	ShaderTechnique::ShaderTechnique() {}

	ShaderTechnique::~ShaderTechnique()
	{
		if(impl_)
		{
			Job::ScopedWriteLock lock(impl_->shader_->rwLock_);

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

	ShaderBindingSet::ShaderBindingSet() {}

	ShaderBindingSet::~ShaderBindingSet()
	{
		if(impl_)
		{
			if(GPU::Manager::IsInitialized())
				GPU::Manager::DestroyResource(impl_->pbs_);
		}
		delete impl_;
	}

	ShaderBindingSet::ShaderBindingSet(ShaderBindingSet&& other)
	{
		std::swap(other.impl_, impl_);
#if !defined(_RELEASE)
		std::swap(other.name_, name_);
#endif
	}

	ShaderBindingSet& ShaderBindingSet::operator=(ShaderBindingSet&& other)
	{
		std::swap(other.impl_, impl_);
#if !defined(_RELEASE)
		std::swap(other.name_, name_);
#endif
		return *this;
	}

	ShaderBindingHandle ShaderBindingSet::GetBindingHandle(const char* name) const
	{
		DBG_ASSERT(impl_);
		auto* factory = Shader::GetFactory();
		if(auto readLock = Core::ScopedReadLock(factory->rwLock_))
		{
			const auto& handles = factory->bindingSetHandles_[impl_->idx_];

			auto it = std::find_if(handles.headers_.begin(), handles.headers_.end(),
			    [name](const ShaderBindingHeader& a) { return strcmp(name, a.name_) == 0; });

			if(it != handles.headers_.end())
			{
				return it->handle_;
			}
		}
		return (ShaderBindingHandle)ShaderBindingFlags::INVALID;
	}

	ShaderBindingSet& ShaderBindingSet::Set(ShaderBindingHandle handle, const GPU::SamplerState& sampler)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(Core::ContainsAllFlags((ShaderBindingFlags)handle, ShaderBindingFlags::SAMPLER));
		const i32 idx = handle & (i32)ShaderBindingFlags::INDEX_MASK;
		if(impl_->samplers_[idx] != sampler && GPU::Manager::IsInitialized())
			GPU::Manager::UpdatePipelineBindings(impl_->pbs_, idx, sampler);
		impl_->samplers_[idx] = sampler;

		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(ShaderBindingHandle handle, const GPU::BindingCBV& binding)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(Core::ContainsAllFlags((ShaderBindingFlags)handle, ShaderBindingFlags::CBV));
		const i32 idx = handle & (i32)ShaderBindingFlags::INDEX_MASK;
		if(impl_->cbvs_[idx] != binding && GPU::Manager::IsInitialized())
			GPU::Manager::UpdatePipelineBindings(impl_->pbs_, idx, binding);
		impl_->cbvs_[idx] = binding;
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(ShaderBindingHandle handle, const GPU::BindingSRV& binding)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(Core::ContainsAllFlags((ShaderBindingFlags)handle, ShaderBindingFlags::SRV));
		const i32 idx = handle & (i32)ShaderBindingFlags::INDEX_MASK;
		if(impl_->srvs_[idx] != binding && GPU::Manager::IsInitialized())
			GPU::Manager::UpdatePipelineBindings(impl_->pbs_, idx, binding);
		impl_->srvs_[idx] = binding;
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(ShaderBindingHandle handle, const GPU::BindingUAV& binding)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(Core::ContainsAllFlags((ShaderBindingFlags)handle, ShaderBindingFlags::UAV));
		const i32 idx = handle & (i32)ShaderBindingFlags::INDEX_MASK;
		if(impl_->uavs_[idx] != binding && GPU::Manager::IsInitialized())
			GPU::Manager::UpdatePipelineBindings(impl_->pbs_, idx, binding);
		impl_->uavs_[idx] = binding;
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::SetAll(const GPU::BindingSRV& binding)
	{
		DBG_ASSERT(impl_);
		for(i32 idx = 0; idx < impl_->srvs_.size(); ++idx)
		{
			if(impl_->srvs_[idx] != binding && GPU::Manager::IsInitialized())
				GPU::Manager::UpdatePipelineBindings(impl_->pbs_, idx, binding);
			impl_->srvs_[idx] = binding;
		}
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(const char* name, const GPU::SamplerState& sampler)
	{
		DBG_ASSERT(impl_);
		if(auto handle = GetBindingHandle(name))
			Set(handle, sampler);
		else
			DBG_LOG("Unable to find binding \"%s\" in ShaderBindingSet \"%s\"\n", name, impl_->header_.name_);
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(const char* name, const GPU::BindingCBV& binding)
	{
		DBG_ASSERT(impl_);
		if(auto handle = GetBindingHandle(name))
			Set(handle, binding);
		else
			DBG_LOG("Unable to find binding \"%s\" in ShaderBindingSet \"%s\"\n", name, impl_->header_.name_);
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(const char* name, const GPU::BindingSRV& binding)
	{
		DBG_ASSERT(impl_);
		if(auto handle = GetBindingHandle(name))
			Set(handle, binding);
		else
			DBG_LOG("Unable to find binding \"%s\" in ShaderBindingSet \"%s\"\n", name, impl_->header_.name_);
		return *this;
	}

	ShaderBindingSet& ShaderBindingSet::Set(const char* name, const GPU::BindingUAV& binding)
	{
		DBG_ASSERT(impl_);
		if(auto handle = GetBindingHandle(name))
			Set(handle, binding);
		else
			DBG_LOG("Unable to find binding \"%s\" in ShaderBindingSet \"%s\"\n", name, impl_->header_.name_);
		return *this;
	}

	bool ShaderBindingSet::Validate() const
	{
		DBG_ASSERT(impl_);
		for(i32 idx = 0; idx < impl_->cbvs_.size(); ++idx)
		{
			if(!impl_->cbvs_[idx].resource_.IsValid())
			{
				DBG_LOG("ShaderBindingSet::Validate: Invalid resource in CBV slot %i\n", idx);
				return false;
			}
		}

		for(i32 idx = 0; idx < impl_->srvs_.size(); ++idx)
		{
			if(!impl_->srvs_[idx].resource_.IsValid())
			{
				DBG_LOG("ShaderBindingSet::Validate: Invalid resource in SRV slot %i\n", idx);
				return false;
			}
		}

		for(i32 idx = 0; idx < impl_->uavs_.size(); ++idx)
		{
			if(!impl_->uavs_[idx].resource_.IsValid())
			{
				DBG_LOG("ShaderBindingSet::Validate: Invalid resource in UAV slot %i\n", idx);
				return false;
			}
		}
		return true;
	}

	ShaderContext::ShaderContext(GPU::CommandList& cmdList)
	{
		auto* factory = Shader::GetFactory();
		if(auto readLock = Core::ScopedReadLock(factory->rwLock_))
		{
			impl_ = new ShaderContextImpl(cmdList);
			impl_->bindingSets_.resize(factory->bindingSetHeaders_.size());
#if !defined(_RELEASE)
			impl_->bindingCallstacks_.resize(impl_->bindingSets_.size());
#endif // !defined(_RELEASE)
		}
	}

	ShaderContext::~ShaderContext() { delete impl_; }

	ShaderContext::ScopedBinding ShaderContext::BeginBindingScope(const ShaderBindingSet& bindingSet)
	{
		if(!bindingSet)
			return ShaderContext::ScopedBinding(*this, -1);

		i32 idx = bindingSet.impl_->idx_;
		DBG_ASSERT(impl_->bindingSets_[idx] == nullptr);
		impl_->bindingSets_[idx] = bindingSet.impl_;

#if !defined(_RELEASE)
		auto& callstack = impl_->bindingCallstacks_[idx];
		Core::GetCallstack(1, callstack.fns_.data(), callstack.fns_.size(), &callstack.hash_);
#endif // !defined(_RELEASE)

		return ShaderContext::ScopedBinding(*this, idx);
	}

	bool ShaderContext::CommitBindings(
	    const ShaderTechnique& tech, GPU::Handle& outPs, Core::ArrayView<GPU::PipelineBinding>& outPb)
	{
		// Count number of required bindings.
		// TODO: This should be packed into the technique itself.
		GPU::PipelineBindingSetDesc tempDesc = {};
		for(const auto& bindingSlot : tech.impl_->header_.bindingSlots_)
		{
			if(bindingSlot.idx_ == -1)
				break;

			const auto* bindingSet = impl_->bindingSets_[bindingSlot.idx_];
#if !defined(_RELEASE)
			if(bindingSet == nullptr)
			{
				const auto* factory = Shader::GetFactory();
				const auto& bindingSetHeader = factory->bindingSetHeaders_[bindingSlot.idx_];
				DBG_LOG("Binding set expected, but not bound: %s\n", bindingSetHeader.name_);
			}
#endif // !defined(_RELEASE)
			DBG_ASSERT(bindingSet != nullptr);

			tempDesc.numCBVs_ = Core::Max(tempDesc.numCBVs_, bindingSlot.cbvReg_ + bindingSet->cbvs_.size());
			tempDesc.numSRVs_ = Core::Max(tempDesc.numSRVs_, bindingSlot.srvReg_ + bindingSet->srvs_.size());
			tempDesc.numUAVs_ = Core::Max(tempDesc.numUAVs_, bindingSlot.uavReg_ + bindingSet->uavs_.size());
			tempDesc.numSamplers_ =
			    Core::Max(tempDesc.numSamplers_, bindingSlot.samplerReg_ + bindingSet->samplers_.size());
		}

		// Allocate pipeline binding.
		GPU::PipelineBinding pb = {};
		pb.pbs_ = GPU::Manager::AllocTemporaryPipelineBindingSet(tempDesc);
		pb.cbvs_.num_ = tempDesc.numCBVs_;
		pb.srvs_.num_ = tempDesc.numSRVs_;
		pb.uavs_.num_ = tempDesc.numUAVs_;
		pb.samplers_.num_ = tempDesc.numSamplers_;

		for(const auto& bindingSlot : tech.impl_->header_.bindingSlots_)
		{
			if(bindingSlot.idx_ == -1)
				break;

			const auto* bindingSet = impl_->bindingSets_[bindingSlot.idx_];
			DBG_ASSERT(bindingSet != nullptr);

			for(const auto& binding : bindingSet->cbvs_)
				DBG_ASSERT(binding.resource_.IsValid());
			for(const auto& binding : bindingSet->srvs_)
				DBG_ASSERT(binding.resource_.IsValid());
			for(const auto& binding : bindingSet->uavs_)
				DBG_ASSERT(binding.resource_.IsValid());

			GPU::PipelineBinding dstPbs = pb;
			dstPbs.cbvs_.num_ = bindingSet->cbvs_.size();
			dstPbs.cbvs_.dstOffset_ = bindingSlot.cbvReg_;
			dstPbs.srvs_.num_ = bindingSet->srvs_.size();
			dstPbs.srvs_.dstOffset_ = bindingSlot.srvReg_;
			dstPbs.uavs_.num_ = bindingSet->uavs_.size();
			dstPbs.uavs_.dstOffset_ = bindingSlot.uavReg_;
			dstPbs.samplers_.num_ = bindingSet->samplers_.size();
			dstPbs.samplers_.dstOffset_ = bindingSlot.samplerReg_;

			GPU::PipelineBinding srcPbs = dstPbs;
			srcPbs.pbs_ = bindingSet->pbs_;
			srcPbs.cbvs_.dstOffset_ = 0;
			srcPbs.cbvs_.srcOffset_ = 0;
			srcPbs.srvs_.dstOffset_ = 0;
			srcPbs.srvs_.srcOffset_ = 0;
			srcPbs.uavs_.dstOffset_ = 0;
			srcPbs.uavs_.srcOffset_ = 0;
			srcPbs.samplers_.dstOffset_ = 0;
			srcPbs.samplers_.srcOffset_ = 0;

			if(tech.impl_->header_.cs_ == -1)
			{
				int a = 0;
				++a;
			}

			GPU::Manager::CopyPipelineBindings(dstPbs, srcPbs);
		}

		GPU::Manager::ValidatePipelineBindings(pb);

		outPb = impl_->cmdList_.Push(Core::ArrayView<GPU::PipelineBinding>(pb));
		outPs = tech.impl_->shader_->pipelineStates_[tech.impl_->descIdx_];
		return true;
	}

	void ShaderContext::EndBindingScope(i32 idx)
	{
		DBG_ASSERT(impl_->bindingSets_[idx] != nullptr);
		impl_->bindingSets_[idx] = nullptr;

#if !defined(_RELEASE)
		impl_->bindingCallstacks_[idx].fns_.fill(nullptr);
#endif // !defined(_RELEASE)
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

	ShaderBindingSetImpl* ShaderImpl::CreateBindingSet(const char* name)
	{
		ShaderBindingSetImpl* bindingSet = nullptr;
		auto* factory = Shader::GetFactory();
		if(auto readLock = Core::ScopedReadLock(factory->rwLock_))
		{
			i32 idx = factory->FindBindingSetIdx(name);
			if(idx >= 0)
			{
				const auto& bindingSetHeader = factory->bindingSetHeaders_[idx];
				if(strcmp(name, bindingSetHeader.name_) == 0)
				{
					bindingSet = new ShaderBindingSetImpl();
					bindingSet->header_ = bindingSetHeader;
					bindingSet->idx_ = idx;

					if(GPU::Manager::IsInitialized())
					{
						GPU::PipelineBindingSetDesc desc;
						desc.shaderVisible_ = false;
						desc.numCBVs_ = bindingSetHeader.numCBVs_;
						desc.numSRVs_ = bindingSetHeader.numSRVs_;
						desc.numUAVs_ = bindingSetHeader.numUAVs_;
						desc.numSamplers_ = bindingSetHeader.numSamplers_;
						bindingSet->pbs_ = GPU::Manager::CreatePipelineBindingSet(
						    desc, "%s/%s", name_.c_str(), bindingSet->header_.name_);
					}

					bindingSet->cbvs_.resize(bindingSetHeader.numCBVs_);
					bindingSet->srvs_.resize(bindingSetHeader.numSRVs_);
					bindingSet->uavs_.resize(bindingSetHeader.numUAVs_);
					bindingSet->samplers_.resize(bindingSetHeader.numSamplers_);
				}
			}
		}
		return bindingSet;
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
			DBG_ASSERT(techHeader->vs_ != -1 || techHeader->cs_ != -1);

			if(techHeader->cs_ != -1)
			{
				GPU::ComputePipelineStateDesc psDesc;
				psDesc.shader_ = shaders_[techHeader->cs_];
				psHandle =
				    GPU::Manager::CreateComputePipelineState(psDesc, "%s/%s", name_.c_str(), impl->header_.name_);
			}
			else
			{
				GPU::GraphicsPipelineStateDesc psDesc;
				psDesc.shaders_[(i32)GPU::ShaderType::VS] =
				    techHeader->vs_ != -1 ? shaders_[techHeader->vs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::HS] =
				    techHeader->hs_ != -1 ? shaders_[techHeader->hs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::DS] =
				    techHeader->ds_ != -1 ? shaders_[techHeader->ds_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::GS] =
				    techHeader->gs_ != -1 ? shaders_[techHeader->gs_] : GPU::Handle();
				psDesc.shaders_[(i32)GPU::ShaderType::PS] =
				    techHeader->ps_ != -1 ? shaders_[techHeader->ps_] : GPU::Handle();
				psDesc.renderState_ = techHeader->rs_;
				psDesc.numVertexElements_ = desc.numVertexElements_;
				memcpy(&psDesc.vertexElements_[0], desc.vertexElements_.data(), sizeof(psDesc.vertexElements_));
				psDesc.topology_ = desc.topology_;
				psDesc.numRTs_ = desc.numRTs_;
				memcpy(&psDesc.rtvFormats_[0], desc.rtvFormats_.data(), sizeof(psDesc.rtvFormats_));
				psDesc.dsvFormat_ = desc.dsvFormat_;
				psHandle =
				    GPU::Manager::CreateGraphicsPipelineState(psDesc, "%s/%s", name_.c_str(), impl->header_.name_);
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

#if 0
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
#endif
		return true;
	}

} // namespace Graphics
