#include "catch.hpp"
#include "test_shared.h"

TEST_CASE("graphics-tests-converter-model")
{
	ScopedEngine engine;

	Graphics::Model* model = nullptr;
	REQUIRE(Resource::Manager::RequestResource(model, "model_tests/teapot.obj"));
	Resource::Manager::WaitForResource(model);
	REQUIRE(Resource::Manager::ReleaseResource(model));
}

TEST_CASE("graphics-tests-converter-shader")
{
	ScopedEngine engine;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);
	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-converter-texture")
{
	ScopedEngine engine;

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
