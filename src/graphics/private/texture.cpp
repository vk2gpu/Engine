#include "graphics/texture.h"
#include "graphics/private/texture_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/misc.h"
#include "core/timer.h"
#include "gpu/manager.h"
#include "gpu/resources.h"
#include "gpu/utils.h"
#include "serialization/serializer.h"

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

			GPU::Handle handle;

			auto CreateTexture = [&](const u8* texData) {
				// Setup subresources.
				i32 numSubRsc = desc.levels_ * desc.elements_;
				Core::Vector<GPU::ConstTextureSubResourceData> subRscs;
				subRscs.reserve(numSubRsc);

				// Should we skip loading mip levels?
				i16 skipMips = skipMips_;
				if(desc.levels_ < skipMips)
				{
					skipMips = desc.levels_ - 1;
				}

				auto formatInfo = GPU::GetFormatInfo(desc.format_);

				i64 texDataOffset = 0;
				for(i32 element = 0; element < desc.elements_; ++element)
				{
					for(i32 level = 0; level < desc.levels_; ++level)
					{
						const auto width = Core::Max(1, desc.width_ >> level);
						const auto height = Core::Max(1, desc.height_ >> level);
						const auto depth = Core::Max(1, desc.depth_ >> level);

						const auto footprint = GPU::GetTextureFootprint(desc.format_, width, height);
						const auto subRscSize = GPU::GetTextureSize(desc.format_, width, height, depth, 1, 1);

						GPU::ConstTextureSubResourceData subRsc;
						subRsc.data_ = texData + texDataOffset;
						subRsc.rowPitch_ = footprint.rowPitch_;
						subRsc.slicePitch_ = footprint.slicePitch_;

						texDataOffset += subRscSize;

						if(level >= skipMips)
						{
							subRscs.push_back(subRsc);
						}
					}
				}

				desc.width_ = Core::Max(1, desc.width_ >> (i32)skipMips);
				desc.height_ = Core::Max(1, desc.height_ >> (i32)skipMips);
				desc.depth_ = Core::Max((i16)1, desc.depth_ >> skipMips);
				desc.levels_ -= skipMips_;

				// Create GPU texture if initialized.
				if(GPU::Manager::IsInitialized())
				{
					handle = GPU::Manager::CreateTexture(desc, subRscs.data(), "%s/%s", name, "texture");
				}
			};

			// Map texture data and create.
			if(auto mapped = Core::MappedFile(inFile, inFile.Tell(), bytes))
			{
				const u8* texData = static_cast<const u8*>(mapped.GetAddress());
				CreateTexture(texData);
			}
			else
			{
				DBG_ASSERT_MSG(false, "FATAL: Unable to map texture data for \"%s\"\n", inFile.GetPath());
				return false;
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

		bool SerializeSettings(Serialization::Serializer& ser) override
		{
			bool retVal = true;
			if(auto object = ser.Object("texture"))
			{
				retVal &= ser.Serialize("skipMips", skipMips_);
			}
			return retVal;
		}


		/**
		 * Settings.
		 */
		i16 skipMips_ = 0;
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
