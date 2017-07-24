#include "graphics/shader.h"
#include "graphics/private/shader_impl.h"

#include "core/debug.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/manager.h"

#define _CRT_DECLARE_NONSTDC_NAMES (0)

#include <algorithm>

namespace Graphics
{
	ShaderTechniqueDesc& ShaderTechniqueDesc::SetVertexElement(i32 idx, const GPU::VertexElement& element)
	{
		numVertexElements_ = Core::Max(numVertexElements_, idx + 1);
		vertexElements_[idx] = element;
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
		}
	}

	ShaderTechniqueImpl* ShaderImpl::CreateTechnique(
	    const char* name, const ShaderTechniqueDesc& desc, ShaderTechniqueImpl* impl)
	{
		// Find valid technique header.
		const ShaderTechniqueHeader* techHeader = nullptr;
		for(const auto& it : techniqueHeaders_)
		{
			if(strcmp(it.name_, name) == 0)
			{
				techHeader = &it;
				break;
			}
		}

		// No name match, exit.
		if(techHeader == nullptr)
			return nullptr;

		// Try to find matching pipeline state for technique.
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

		if(foundIdx == -1)
		{
			Core::Array<char, 128> debugName;
			sprintf_s(debugName.data(), debugName.size(), "%s/%s", name_.c_str(), name);

			// Create pipeline state for technique.
			GPU::Handle psHandle;
			if(GPU::Manager::IsInitialized())
			{
				if(techHeader->cs_ != -1)
				{
					GPU::ComputePipelineStateDesc psDesc;
					psDesc.shader_ = shaders_[techHeader->cs_];
					psHandle = GPU::Manager::CreateComputePipelineState(psDesc, debugName.data());
				}
				else
				{
					GPU::GraphicsPipelineStateDesc psDesc;
					psDesc.shaders_[(i32)GPU::ShaderType::VERTEX] =
					    techHeader->vs_ != -1 ? shaders_[techHeader->vs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::GEOMETRY] =
					    techHeader->gs_ != -1 ? shaders_[techHeader->gs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::HULL] =
					    techHeader->hs_ != -1 ? shaders_[techHeader->hs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::DOMAIN] =
					    techHeader->ds_ != -1 ? shaders_[techHeader->ds_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::PIXEL] =
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

				// Check the pipeline state is valid.
				if(!psHandle)
				{
					return nullptr;
				}
			}

			techniqueDescHashes_.push_back(hash);
			techniqueDescs_.push_back(desc);
			pipelineStates_.push_back(psHandle);
			foundIdx = pipelineStates_.size() - 1;
		}

		// Setup impl.
		ShaderTechnique tech;
		ShaderTechniqueImpl* techImpl = impl ? impl : new ShaderTechniqueImpl;

		techImpl->shader_ = this;
		techImpl->header_ = techHeader;
		techImpl->descIdx_ = foundIdx;
		techImpl->cbvs_.resize(header_.numCBuffers_);
		techImpl->samplers_.resize(header_.numSamplers_);
		techImpl->srvs_.resize(header_.numSRVs_);
		techImpl->uavs_.resize(header_.numUAVs_);
		techImpl->bs_.pipelineState_ = pipelineStates_[foundIdx];
		techImpl->bsDirty_ = true;

		// Calculate offset into vectors for setting.
		techImpl->samplerOffset_ = techImpl->cbvs_.size();
		techImpl->srvOffset_ = techImpl->samplers_.size() + techImpl->samplerOffset_;
		techImpl->uavOffset_ = techImpl->srvs_.size() + techImpl->srvOffset_;

		techniques_.push_back(techImpl);

		return techImpl;
	}


	Shader::Shader()
	{ //
	}

	Shader::~Shader()
	{ //
		DBG_ASSERT(impl_);
		delete impl_;
	}

	i32 Shader::GetBindingIndex(const char* name) const
	{
		for(i32 idx = 0; idx < impl_->bindingHeaders_.size(); ++idx)
		{
			const auto& binding = impl_->bindingHeaders_[idx];
			if(strcmp(binding.name_, name) == 0)
				return idx;
		}
		return -1;
	}

	ShaderTechnique Shader::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		ShaderTechnique tech;
		tech.impl_ = impl_->CreateTechnique(name, desc, nullptr);
		return tech;
	}


	ShaderTechnique::ShaderTechnique() {}

	ShaderTechnique::~ShaderTechnique()
	{
		if(impl_)
		{
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

	void ShaderTechnique::SetCBV(i32 idx, GPU::Handle res, i32 offset, i32 size)
	{
		DBG_ASSERT(idx >= 0 && idx < impl_->cbvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->cbvs_[idx];
		binding.resource_ = res;
		binding.offset_ = offset;
		binding.size_ = size;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetSampler(i32 idx, GPU::Handle res)
	{
		idx -= impl_->samplerOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->samplers_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->samplers_[idx];
		binding.resource_ = res;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetBuffer(
	    i32 idx, GPU::Handle res, i32 firstElement, i32 numElements, i32 structureByteStride)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::BUFFER;
		binding.mostDetailedMip_FirstElement_ = firstElement;
		binding.mipLevels_NumElements_ = numElements;
		DBG_ASSERT(structureByteStride == 0); // TODO.
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTexture1D(
	    i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX1D;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTexture1DArray(i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels,
	    i32 firstArraySlice, i32 arraySize, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX1D_ARRAY;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.firstArraySlice_ = firstArraySlice;
		binding.arraySize_ = arraySize;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTexture2D(
	    i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels, i32 planeSlice, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX2D;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.planeSlice_ = planeSlice;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTexture2DArray(i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels,
	    i32 firstArraySlice, i32 arraySize, i32 planeSlice, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX2D_ARRAY;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.firstArraySlice_ = firstArraySlice;
		binding.arraySize_ = arraySize;
		binding.planeSlice_ = planeSlice;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTexture3D(
	    i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX3D;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTextureCube(
	    i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEXCUBE;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetTextureCubeArray(i32 idx, GPU::Handle res, i32 mostDetailedMip, i32 mipLevels,
	    i32 first2DArrayFace, i32 numCubes, f32 resourceMinLODClamp)
	{
		idx -= impl_->srvOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->srvs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->srvs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEXCUBE_ARRAY;
		binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
		binding.mipLevels_NumElements_ = mipLevels;
		binding.firstArraySlice_ = first2DArrayFace;
		binding.arraySize_ = numCubes;
		binding.resourceMinLODClamp_ = resourceMinLODClamp;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWBuffer(
	    i32 idx, GPU::Handle res, i32 firstElement, i32 numElements, i32 structuredByteSize)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::BUFFER;
		binding.mipSlice_FirstElement_ = firstElement;
		binding.firstArraySlice_FirstWSlice_NumElements_ = numElements;
		DBG_ASSERT(structuredByteSize == 0); // TODO.
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWTexture1D(i32 idx, GPU::Handle res, i32 mipSlice)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX1D;
		binding.mipSlice_FirstElement_ = mipSlice;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWTexture1DArray(
	    i32 idx, GPU::Handle res, i32 mipSlice, i32 firstArraySlice, i32 arraySize)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX1D_ARRAY;
		binding.mipSlice_FirstElement_ = mipSlice;
		binding.firstArraySlice_FirstWSlice_NumElements_ = firstArraySlice;
		binding.arraySize_PlaneSlice_WSize_ = arraySize;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWTexture2D(i32 idx, GPU::Handle res, i32 mipSlice, i32 planeSlice)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX2D;
		binding.mipSlice_FirstElement_ = mipSlice;
		binding.arraySize_PlaneSlice_WSize_ = planeSlice;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWTexture2DArray(
	    i32 idx, GPU::Handle res, i32 mipSlice, i32 planeSlice, i32 firstArraySlice, i32 arraySize)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX2D_ARRAY;
		binding.mipSlice_FirstElement_ = mipSlice;
		binding.arraySize_PlaneSlice_WSize_ = planeSlice;
		binding.firstArraySlice_FirstWSlice_NumElements_ = firstArraySlice;
		binding.arraySize_PlaneSlice_WSize_ = arraySize;
		impl_->bsDirty_ = true;
	}

	void ShaderTechnique::SetRWTexture3D(i32 idx, GPU::Handle res, i32 mipSlice, i32 firstWSlice, i32 wSize)
	{
		idx -= impl_->uavOffset_;
		DBG_ASSERT(idx >= 0 && idx < impl_->uavs_.size());
		DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(res));
		auto& binding = impl_->uavs_[idx];
		binding.resource_ = res;
		binding.dimension_ = GPU::ViewDimension::TEX3D;
		binding.mipSlice_FirstElement_ = mipSlice;
		binding.firstArraySlice_FirstWSlice_NumElements_ = firstWSlice;
		binding.arraySize_PlaneSlice_WSize_ = wSize;
		impl_->bsDirty_ = true;
	}

	GPU::Handle ShaderTechnique::GetBinding()
	{
		if(impl_ && impl_->bsDirty_)
		{
			if(impl_->bsHandle_)
				GPU::Manager::DestroyResource(impl_->bsHandle_);

			// Setup binding set with offsets that come from shaders.
			const auto SetupCBV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
				impl_->bs_.numCBVs_ = numMappings;
				for(i32 idx = 0; idx < numMappings; ++idx)
				{
					impl_->bs_.cbvs_[mapping[idx].dstSlot_] = impl_->cbvs_[mapping[idx].binding_];
				}
				return mapping + numMappings;
			};

			const auto SetupSampler = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
				impl_->bs_.numSamplers_ = numMappings;
				for(i32 idx = 0; idx < numMappings; ++idx)
				{
					impl_->bs_.samplers_[mapping[idx].dstSlot_] =
					    impl_->samplers_[mapping[idx].binding_ - impl_->samplerOffset_];
				}
				return mapping + numMappings;
			};

			const auto SetupSRV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
				impl_->bs_.numSRVs_ = numMappings;
				for(i32 idx = 0; idx < numMappings; ++idx)
				{
					impl_->bs_.srvs_[mapping[idx].dstSlot_] = impl_->srvs_[mapping[idx].binding_ - impl_->srvOffset_];
				}
				return mapping + numMappings;
			};

			const auto SetupUAV = [this](const ShaderBindingMapping* mapping, i32 numMappings) {
				impl_->bs_.numUAVs_ = numMappings;
				for(i32 idx = 0; idx < numMappings; ++idx)
				{
					impl_->bs_.uavs_[mapping[idx].dstSlot_] = impl_->uavs_[mapping[idx].binding_ - impl_->uavOffset_];
				}
				return mapping + numMappings;
			};

			const auto SetupBindings = [&](i32 shaderIdx) {
				if(shaderIdx < 0)
					return;

				const auto& bytecode = impl_->shader_->bytecodeHeaders_[shaderIdx];
				const auto* mappings = impl_->shader_->shaderBindingMappings_[shaderIdx];

				mappings = SetupCBV(mappings, bytecode.numCBuffers_);
				mappings = SetupSampler(mappings, bytecode.numSamplers_);
				mappings = SetupSRV(mappings, bytecode.numSRVs_);
				mappings = SetupUAV(mappings, bytecode.numUAVs_);

			};

			SetupBindings(impl_->header_->vs_);
			SetupBindings(impl_->header_->gs_);
			SetupBindings(impl_->header_->hs_);
			SetupBindings(impl_->header_->ds_);
			SetupBindings(impl_->header_->ps_);
			SetupBindings(impl_->header_->cs_);

			Core::Array<char, 128> debugName;
			sprintf_s(debugName.data(), debugName.size(), "%s/%s_binding", impl_->shader_->name_.c_str(),
			    impl_->header_->name_);
			impl_->bsHandle_ = GPU::Manager::CreatePipelineBindingSet(impl_->bs_, debugName.data());
			DBG_ASSERT(impl_->bsHandle_);
			DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(impl_->bsHandle_));
			impl_->bsDirty_ = false;
		}
		return impl_->bsHandle_;
	}


} // namespace Graphics
