#include "graphics/texture.h"
#include "graphics/private/texture_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/misc.h"
#include "gpu/manager.h"
#include "gpu/resources.h"
#include "gpu/utils.h"

namespace Graphics
{
	class TextureFactory : public Resource::IFactory
	{
	public:
		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Texture::GetTypeUUID());
			*outResource = new Texture();
			return true;
		}

		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Texture::GetTypeUUID());
			auto* texxture = reinterpret_cast<Texture*>(*inResource);
			delete texxture;
			*inResource = nullptr;
			return true;
		}

		bool LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
		    const char* name, Core::File& inFile) override
		{
			Texture* texture = *reinterpret_cast<Texture**>(inResource);
			DBG_ASSERT(texture);

			const bool isReload = texture->IsReady();

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

			// Create impl.
			auto impl = new TextureImpl();
			impl->desc_ = desc;
			impl->handle_ = handle;

			if(isReload)
			{
				auto lock = Resource::Manager::TakeReloadLock();
				std::swap(texture->impl_, impl);
				delete impl;
			}
			else
			{
				std::swap(texture->impl_, impl);
				DBG_ASSERT(impl == nullptr);
			}

			return true;
		}
	};

	DEFINE_RESOURCE(Texture);

	Texture::Texture()
	{ //
	}

	Texture::~Texture()
	{
		DBG_ASSERT(impl_);
		delete impl_;
	}

	const GPU::TextureDesc& Texture::GetDesc() const
	{
		DBG_ASSERT(impl_);
		return impl_->desc_;
	}

	GPU::Handle Texture::GetHandle() const
	{
		DBG_ASSERT(impl_);
		return impl_->handle_;
	}

	TextureImpl::~TextureImpl()
	{
		if(GPU::Manager::IsInitialized())
		{
			GPU::Manager::DestroyResource(handle_);
		}
	}

} // namespace Graphics
