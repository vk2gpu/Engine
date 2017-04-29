#include "graphics/factory.h"
#include "graphics/texture.h"
#include "graphics/private/texture_impl.h"

#include "core/file.h"
#include "core/misc.h"

#include "gpu/resources.h"
#include "gpu/utils.h"

#include "gpu/manager.h"

#include <memory>
#include <utility>

namespace Graphics
{
	bool Factory::CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type)
	{
		if(type == Texture::GetTypeUUID())
		{
			*outResource = new Texture();
			return true;
		}
		
		return false;
	}

	bool Factory::LoadResource(
	    Resource::IFactoryContext& context, void** inResource, const Core::UUID& type, Core::File& inFile)
	{ //
		if(type == Texture::GetTypeUUID())
		{
			return LoadTexture(context, *reinterpret_cast<Texture**>(inResource), type, inFile);
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

		return false;
	}

	bool Factory::LoadTexture(
	    Resource::IFactoryContext& context, Texture* inResource, const Core::UUID& type, Core::File& inFile)
	{
		GPU::TextureDesc desc;

		// Read in desc.
		if(inFile.Read(&desc, sizeof(desc)) != sizeof(desc))
		{
			return false;
		}
		i64 bytes = GPU::GetTextureSize(desc.format_, desc.width_, desc.height_, desc.depth_, desc.levels_, desc.elements_);

		// Allocate bytes to read in.
		// TODO: Implement a Map/Unmap interface on Core::File to allow reading in-place or memory mapping.
		std::unique_ptr<u8> texData(new u8[bytes]);
		memset(texData.get(), 0, bytes);
		
		// Read texture data in.
		inFile.Read(texData.get(), bytes);

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

				const auto texLayoutInfo = GPU::GetTextureLayoutInfo(desc.format_, desc.width_, desc.height_);
				const auto subRscSize = GPU::GetTextureSize(desc.format_, desc.width_, desc.height_, desc.depth_, 1, 1);

				GPU::TextureSubResourceData subRsc;
				subRsc.data_ = texData.get() + texDataOffset;
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
			handle = GPU::Manager::CreateTexture(desc, subRscs.data(), "TODO NAME");
		}

		// Finish creating texture.
		inResource->impl_ = new TextureImpl();
		inResource->impl_->desc_ = desc;
		inResource->impl_->handle_ = handle;

		return true;
	}

} // namespace Graphics
