#include "graphics/material.h"
#include "graphics/texture.h"
#include "graphics/private/material_impl.h"
#include "core/file.h"
#include "resource/factory.h"
#include "resource/manager.h"

namespace Graphics
{
	class MaterialFactory : public Resource::IFactory
	{
	public:
		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Material::GetTypeUUID());
			*outResource = new Material();
			return true;
		}

		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Material::GetTypeUUID());
			auto* shader = reinterpret_cast<Material*>(*inResource);
			delete shader;
			*inResource = nullptr;
			return true;
		}

		bool LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
		    const char* name, Core::File& inFile) override
		{
			Material* material = *reinterpret_cast<Material**>(inResource);
			i64 readBytes = 0;

			DBG_ASSERT(material);

			auto* impl = new MaterialImpl();

			readBytes = sizeof(MaterialData);
			if(inFile.Read(&impl->data_, readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			impl->textures_.resize(impl->data_.numTextures_);
			readBytes = impl->data_.numTextures_ * sizeof(MaterialTexture);
			if(inFile.Read(impl->textures_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			// Attempt to load in all dependent resources.
			impl->shaderRes_ = ShaderRef(impl->data_.shader_);
			impl->textureRes_.reserve(impl->data_.numTextures_);
			for(i32 idx = 0; idx < impl->data_.numTextures_; ++idx)
			{
				impl->textureRes_.emplace_back(impl->textures_[idx].resourceName_);
			}

			// Wait on resources to finish loading.
			impl->shaderRes_.WaitUntilReady();
			for(const auto& textureRes : impl->textureRes_)
			{
				if(textureRes)
					textureRes.WaitUntilReady();
			}

			material->impl_ = impl;

			return true;
		}
	};

	DEFINE_RESOURCE(Material);

	Shader* Material::GetShader() const { return impl_->shaderRes_; }

	ShaderTechnique Material::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		auto tech = impl_->shaderRes_->CreateTechnique(name, desc);

		// Set textures.
		for(i32 idx = 0; idx < impl_->textures_.size(); ++idx)
		{
			const auto& bindingName = impl_->textures_[idx].bindingName_;
			auto bindingIdx = impl_->shaderRes_->GetBindingIndex(bindingName.data());
			if(bindingIdx != -1)
			{
				Texture* tex = impl_->textureRes_[idx];
				if(tex)
				{
					const auto& texDesc = tex->GetDesc();
					switch(texDesc.type_)
					{
					case GPU::TextureType::TEX1D:
						tech.Set(
							bindingIdx, GPU::Binding::Texture1D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
						break;
					case GPU::TextureType::TEX2D:
						tech.Set(
							bindingIdx, GPU::Binding::Texture2D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
						break;
					case GPU::TextureType::TEX3D:
						tech.Set(
							bindingIdx, GPU::Binding::Texture3D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
						break;
					case GPU::TextureType::TEXCUBE:
						tech.Set(
							bindingIdx, GPU::Binding::TextureCube(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
						break;
					}
				}
			}
		}
		return tech;
	}

	Material::Material() {}

	Material::~Material() {}

	MaterialImpl::MaterialImpl() {}

	MaterialImpl::~MaterialImpl() {}

} // namespace Graphics
