#include "graphics/factory.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/private/shader_impl.h"
#include "graphics/private/texture_impl.h"

#include "core/file.h"
#include "core/misc.h"

#include "gpu/resources.h"
#include "gpu/utils.h"

#include "gpu/manager.h"

#include <utility>

namespace Graphics
{
	bool Factory::CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type)
	{
		if(type == Shader::GetTypeUUID())
		{
			*outResource = new Shader();
			return true;
		}
		else if(type == Texture::GetTypeUUID())
		{
			*outResource = new Texture();
			return true;
		}

		return false;
	}

	bool Factory::LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
	    const char* name, Core::File& inFile)
	{
		if(type == Shader::GetTypeUUID())
		{
			return LoadShader(context, *reinterpret_cast<Shader**>(inResource), type, name, inFile);
		}
		else if(type == Texture::GetTypeUUID())
		{
			return LoadTexture(context, *reinterpret_cast<Texture**>(inResource), type, name, inFile);
		}


		return false;
	}

	bool Factory::DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type)
	{
		if(type == Texture::GetTypeUUID())
		{
			auto* texture = reinterpret_cast<Texture*>(*inResource);
			delete texture;
			*inResource = nullptr;
			return true;
		}

		else if(type == Shader::GetTypeUUID())
		{
			auto* texture = reinterpret_cast<Shader*>(*inResource);
			delete texture;
			*inResource = nullptr;
			return true;
		}

		return false;
	}

	bool Factory::LoadShader(Resource::IFactoryContext& context, Shader* inResource, const Core::UUID& type,
	    const char* name, Core::File& inFile)
	{
		const bool isReload = inResource->IsReady();

		if(isReload)
		{
			DBG_ASSERT(inResource->impl_);
			inResource->impl_->reloadLock_.Lock();
		}

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

		impl->bindingHeaders_.resize(header.numCBuffers_ + header.numSamplers_ + header.numSRVs_ + header.numUAVs_);
		readBytes = impl->bindingHeaders_.size() * sizeof(ShaderBindingHeader);
		if(inFile.Read(impl->bindingHeaders_.data(), readBytes) != readBytes)
		{
			return false;
		}

		impl->bytecodeHeaders_.resize(header.numShaders_);
		readBytes = impl->bytecodeHeaders_.size() * sizeof(ShaderBytecodeHeader);
		if(inFile.Read(impl->bytecodeHeaders_.data(), readBytes) != readBytes)
		{
			return false;
		}

		i32 numBindingMappings = 0;
		i32 bytecodeSize = 0;
		for(const auto& bytecodeHeader : impl->bytecodeHeaders_)
		{
			numBindingMappings += bytecodeHeader.numCBuffers_ + bytecodeHeader.numSamplers_ + bytecodeHeader.numSRVs_ +
			                      bytecodeHeader.numUAVs_;
			bytecodeSize = Core::Max(bytecodeSize, bytecodeHeader.offset_ + bytecodeHeader.numBytes_);
		}

		impl->bindingMappings_.resize(numBindingMappings);
		readBytes = impl->bindingMappings_.size() * sizeof(ShaderBindingMapping);
		if(inFile.Read(impl->bindingMappings_.data(), readBytes) != readBytes)
		{
			return false;
		}

		impl->techniqueHeaders_.resize(header.numTechniques_);
		readBytes = impl->techniqueHeaders_.size() * sizeof(ShaderTechniqueHeader);
		if(inFile.Read(impl->techniqueHeaders_.data(), readBytes) != readBytes)
		{
			return false;
		}

		impl->bytecode_.resize(bytecodeSize);
		readBytes = impl->bytecode_.size();
		if(inFile.Read(impl->bytecode_.data(), readBytes) != readBytes)
		{
			return false;
		}

		// Create all the shaders.
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
					for(auto s : impl->shaders_)
						GPU::Manager::DestroyResource(s);
					delete impl;
					return false;
				}

				impl->shaders_.push_back(handle);

				impl->shaderBindingMappings_.push_back(mapping);

				mapping += (bytecode.numCBuffers_ + bytecode.numSamplers_ + bytecode.numSRVs_ + bytecode.numUAVs_);
			}

			// Bytecode no longer needed once created.
			impl->bytecode_.clear();
		}

		if(isReload)
		{
			// Setup technique descs, hashes, and empty pipeline states.
			std::swap(impl->techniqueDescHashes_, inResource->impl_->techniqueDescHashes_);
			std::swap(impl->techniqueDescs_, inResource->impl_->techniqueDescs_);
			impl->pipelineStates_.resize(impl->techniqueDescs_.size());

			// Swap techniques over.
			std::swap(impl->techniques_, inResource->impl_->techniques_);

			// Setup techniques again in their new impl.
			for(i32 idx = 0; idx < impl->techniques_.size(); ++idx)
			{
				auto* techImpl = impl->techniques_[idx];
				techImpl->shader_ = impl;
				impl->SetupTechnique(techImpl);
			}

			// Swap in new resource impl.
			std::swap(inResource->impl_, impl);

			// Release reload lock and delete.
			impl->reloadLock_.Unlock();
			delete impl;
		}
		else
		{
			inResource->impl_ = impl;
		}

		return true;
	}

	bool Factory::LoadTexture(Resource::IFactoryContext& context, Texture* inResource, const Core::UUID& type,
	    const char* name, Core::File& inFile)
	{
		const bool isReload = inResource->IsReady();
		DBG_ASSERT(!isReload);

		GPU::TextureDesc desc;

		// Read in desc.
		if(inFile.Read(&desc, sizeof(desc)) != sizeof(desc))
		{
			return false;
		}
		i64 bytes =
		    GPU::GetTextureSize(desc.format_, desc.width_, desc.height_, desc.depth_, desc.levels_, desc.elements_);

		// Allocate bytes to read in.
		// TODO: Implement a Map/Unmap interface on Core::File to allow reading in-place or memory mapping.
		Core::Vector<u8> texData((i32)bytes);
		memset(texData.data(), 0, texData.size());

		// Read texture data in.
		inFile.Read(texData.data(), texData.size());

		// Setup subresources.
		i32 numSubRsc = desc.levels_ * desc.elements_;
		Core::Vector<GPU::TextureSubResourceData> subRscs;
		subRscs.reserve(numSubRsc);

		i64 texDataOffset = 0;
		for(i32 element = 0; element < desc.elements_; ++element)
		{
			for(i32 level = 0; level < desc.levels_; ++level)
			{
				const auto width = Core::Max(1, desc.width_ >> level);
				const auto height = Core::Max(1, desc.height_ >> level);
				const auto depth = Core::Max(1, desc.depth_ >> level);

				const auto texLayoutInfo = GPU::GetTextureLayoutInfo(desc.format_, width, height);
				const auto subRscSize = GPU::GetTextureSize(desc.format_, width, height, depth, 1, 1);

				GPU::TextureSubResourceData subRsc;
				subRsc.data_ = texData.data() + texDataOffset;
				subRsc.rowPitch_ = texLayoutInfo.pitch_;
				subRsc.slicePitch_ = texLayoutInfo.slicePitch_;

				texDataOffset += subRscSize;
				subRscs.push_back(subRsc);
			}
		}

		// Create GPU texture if initialized.
		GPU::Handle handle;
		if(GPU::Manager::IsInitialized())
		{
			handle = GPU::Manager::CreateTexture(desc, subRscs.data(), name);
		}

		// Finish creating texture.
		inResource->impl_ = new TextureImpl();
		inResource->impl_->desc_ = desc;
		inResource->impl_->handle_ = handle;

		return true;
	}

} // namespace Graphics
