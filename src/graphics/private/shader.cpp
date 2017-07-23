#include "graphics/shader.h"
#include "graphics/private/shader_impl.h"

#include "core/debug.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/manager.h"

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

	Shader::Shader()
	{ //
	}

	Shader::~Shader()
	{ //
		DBG_ASSERT(impl_);
		DBG_ASSERT_MSG(impl_->liveTechniques_ == 0, "Techniques still reference this shader.");

		if(GPU::Manager::IsInitialized())
		{
			for(auto ps : impl_->pipelineStates_)
				GPU::Manager::DestroyResource(ps);
			for(auto s : impl_->shaders_)
				GPU::Manager::DestroyResource(s);
		}
		delete impl_;
	}

	ShaderTechnique Shader::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		// Find valid technique header.
		const ShaderTechniqueHeader* techHeader = nullptr;
		for(const auto& it : impl_->techniqueHeaders_)
		{
			if(strcmp(it.name_, name) == 0)
			{
				techHeader = &it;
				break;
			}
		}

		// No name match, exit.
		if(techHeader == nullptr)
			return ShaderTechnique();

		// Try to find matching pipeline state for technique.
		i32 foundIdx = -1;
		u32 hash = Core::HashCRC32(0, &desc, sizeof(desc));
		hash = Core::Hash(hash, name);
		for(i32 idx = 0; idx < impl_->techniqueDescHashes_.size(); ++idx)
		{
			if(impl_->techniqueDescHashes_[idx] == hash)
			{
				DBG_ASSERT_MSG(impl_->techniqueDescs_[idx] == desc, "Technique hash collision!");
				foundIdx = idx;
			}
		}

		if(foundIdx == -1)
		{
			Core::Array<char, 128> debugName;
			sprintf_s(debugName.data(), debugName.size(), "%s/%s", impl_->name_.c_str(), name);

			// Create pipeline state for technique.
			GPU::Handle psHandle;
			if(GPU::Manager::IsInitialized())
			{
				if(techHeader->cs_ != -1)
				{
					GPU::ComputePipelineStateDesc psDesc;
					psDesc.shader_ = impl_->shaders_[techHeader->cs_];
					psHandle = GPU::Manager::CreateComputePipelineState(psDesc, debugName.data());
				}
				else
				{
					GPU::GraphicsPipelineStateDesc psDesc;
					psDesc.shaders_[(i32)GPU::ShaderType::VERTEX] =
					    techHeader->vs_ != -1 ? impl_->shaders_[techHeader->vs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::GEOMETRY] =
					    techHeader->gs_ != -1 ? impl_->shaders_[techHeader->gs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::HULL] =
					    techHeader->hs_ != -1 ? impl_->shaders_[techHeader->hs_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::DOMAIN] =
					    techHeader->ds_ != -1 ? impl_->shaders_[techHeader->ds_] : GPU::Handle();
					psDesc.shaders_[(i32)GPU::ShaderType::PIXEL] =
					    techHeader->ps_ != -1 ? impl_->shaders_[techHeader->ps_] : GPU::Handle();
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
					return ShaderTechnique();
				}
			}

			impl_->techniqueDescHashes_.push_back(hash);
			impl_->techniqueDescs_.push_back(desc);
			impl_->pipelineStates_.push_back(psHandle);
			foundIdx = impl_->pipelineStates_.size() - 1;
		}

		// Setup impl.
		ShaderTechnique tech;
		ShaderTechniqueImpl* techImpl = new ShaderTechniqueImpl;

		techImpl->shader_ = impl_;
		techImpl->header_ = techHeader;
		techImpl->bs_.pipelineState_ = impl_->pipelineStates_[foundIdx];

		tech.impl_ = techImpl;
		impl_->liveTechniques_++;

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
			impl_->shader_->liveTechniques_--;
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

	GPU::Handle ShaderTechnique::GetBinding()
	{
		if(impl_ && impl_->bsDirty_)
		{
			if(impl_->bsHandle_)
				GPU::Manager::DestroyResource(impl_->bsHandle_);

			Core::Array<char, 128> debugName;
			sprintf_s(debugName.data(), debugName.size(), "%s/%s_binding", impl_->shader_->name_.c_str(),
			    impl_->header_->name_);
			impl_->bsHandle_ = GPU::Manager::CreatePipelineBindingSet(impl_->bs_, debugName.data());
			DBG_ASSERT(impl_->bsHandle_);
			DBG_ASSERT(GPU::Manager::GetHandleAllocator().IsValid(impl_->bsHandle_));
		}
		return impl_->bsHandle_;
	}


} // namespace Graphics
