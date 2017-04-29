#pragma once

#include "graphics/dll.h"
#include "resource/factory.h"

namespace Graphics
{
	class GRAPHICS_DLL Factory : public Resource::IFactory
	{
	public:
		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override;
		bool LoadResource(
		    Resource::IFactoryContext& context, void** inResource, const Core::UUID& type, Core::File& inFile) override;
		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override;

	private:
		bool LoadTexture(
		    Resource::IFactoryContext& context, class Texture* inResource, const Core::UUID& type, Core::File& inFile);
	};
} // namespace Graphics
