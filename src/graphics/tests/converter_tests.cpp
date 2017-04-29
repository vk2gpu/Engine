#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "resource/manager.h"

#include "graphics/factory.h"
#include "graphics/texture.h"


namespace
{
	class ScopedFactory
	{
	public:
		ScopedFactory()
		{
			factory_ = new Graphics::Factory();
			Resource::Manager::RegisterFactory<Graphics::Texture>(factory_);
		}

		~ScopedFactory()
		{
			Resource::Manager::UnregisterFactory(factory_);
			delete factory_;
		}

	private:
		Resource::IFactory* factory_ = nullptr;
	};
}

TEST_CASE("graphics-tests-converter-texture")
{
	Plugin::Manager::Scoped pluginManager;
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	Graphics::Texture* texture = nullptr;

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture.png"));

	Resource::Manager::WaitForResource(texture);

	REQUIRE(Resource::Manager::ReleaseResource(texture));
}
