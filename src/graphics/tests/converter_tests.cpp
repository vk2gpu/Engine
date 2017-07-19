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
#include "graphics/shader.h"
#include "graphics/texture.h"


namespace
{
	class ScopedFactory
	{
	public:
		ScopedFactory()
		{
			factory_ = new Graphics::Factory();
			Resource::Manager::RegisterFactory<Graphics::Shader>(factory_);
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

TEST_CASE("graphics-tests-converter-shader")
{
	Plugin::Manager::Scoped pluginManager;
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);
	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-converter-texture")
{
	Plugin::Manager::Scoped pluginManager;
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	Graphics::Texture* texture = nullptr;

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_png.png"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_jpg.jpg"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_tga.tga"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_bc3.dds"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_bc6.dds"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));

	REQUIRE(Resource::Manager::RequestResource(texture, "test_texture_bc7.dds"));
	Resource::Manager::WaitForResource(texture);
	REQUIRE(Resource::Manager::ReleaseResource(texture));
}
