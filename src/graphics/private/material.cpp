#include "graphics/material.h"
#include "graphics/texture.h"
#include "graphics/private/material_impl.h"
#include "gpu/manager.h"
#include "core/file.h"
#include "resource/factory.h"
#include "resource/manager.h"

namespace Graphics
{
	class MaterialFactory : public Resource::IFactory
	{
	public:
		~MaterialFactory()
		{
			if(defaultTex_)
			{
				GPU::Manager::DestroyResource(defaultTex_);
			}
		}

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

			if(!defaultTex_ && GPU::Manager::IsInitialized())
			{
				GPU::TextureDesc desc;
				desc.type_ = GPU::TextureType::TEX2D;
				desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
				desc.format_ = GPU::Format::R8G8B8A8_UNORM;
				desc.width_ = 4;
				desc.height_ = 4;
				defaultTex_ = GPU::Manager::CreateTexture(desc, nullptr, "MaterialFactory/defaultTex");
				DBG_ASSERT(defaultTex_);
			}

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

			// Wait on resources to finish loading.
			impl->shaderRes_.WaitUntilReady();
			for(const auto& textureRes : impl->textureRes_)
			{
				if(textureRes)
					textureRes.WaitUntilReady();
			}

			// Setup material bindings, if there are any.
			if(impl->bindings_ = impl->shaderRes_->CreateBindingSet("MaterialBindings"))
			{
				// Set default texture for everything. Temporary workaround.
				impl->bindings_.SetAll(GPU::Binding::Texture2D(defaultTex_, GPU::Format::R8G8B8A8_UNORM, 0, 1));

				//
				for(i32 idx = 0; idx < impl->data_.numTextures_; ++idx)
				{
					impl->textureRes_.emplace_back(impl->textures_[idx].resourceName_);
					if(impl->textureRes_.back())
					{
						impl->textureRes_.back().WaitUntilReady();

						const char* bindingName = impl->textures_[idx].bindingName_.data();
						Texture* tex = impl->textureRes_[idx];
						if(tex)
						{
							const auto& texDesc = tex->GetDesc();
							switch(texDesc.type_)
							{
							case GPU::TextureType::TEX1D:
								impl->bindings_.Set(bindingName,
								    GPU::Binding::Texture1D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
								break;
							case GPU::TextureType::TEX2D:
								impl->bindings_.Set(bindingName,
								    GPU::Binding::Texture2D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
								break;
							case GPU::TextureType::TEX3D:
								impl->bindings_.Set(bindingName,
								    GPU::Binding::Texture3D(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
								break;
							case GPU::TextureType::TEXCUBE:
								impl->bindings_.Set(bindingName,
								    GPU::Binding::TextureCube(tex->GetHandle(), texDesc.format_, 0, texDesc.levels_));
								break;
							}
						}
					}
				}

				DBG_ASSERT(impl->bindings_.Validate());
			}

			impl->name_ = name;
			material->impl_ = impl;

			return true;
		}

		bool SerializeSettings(Serialization::Serializer& ser) override { return true; }


		GPU::Handle defaultTex_;
	};

	DEFINE_RESOURCE(Material);

	const char* Material::GetName() const { return impl_->name_.c_str(); }

	Shader* Material::GetShader() const { return impl_->shaderRes_; }

	const ShaderBindingSet& Material::GetBindingSet() const { return impl_->bindings_; }

	ShaderTechnique Material::CreateTechnique(const char* name, const ShaderTechniqueDesc& desc)
	{
		auto tech = impl_->shaderRes_->CreateTechnique(name, desc);
		return tech;
	}

	Material::Material() {}

	Material::~Material() { delete impl_; }

	MaterialImpl::MaterialImpl() {}

	MaterialImpl::~MaterialImpl() {}

} // namespace Graphics
