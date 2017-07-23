#include "catch.hpp"
#include "test_shared.h"

TEST_CASE("graphics-tests-converter-shader")
{
	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	// Init device.
	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);
	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-converter-texture")
{
	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	// Init device.
	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

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
