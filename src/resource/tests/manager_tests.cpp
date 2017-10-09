#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/misc.h"
#include "core/uuid.h"
#include "core/vector.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "resource/converter.h"
#include "resource/factory.h"
#include "resource/manager.h"
#include "resource/ref.h"
#include "resource/resource.h"

#include <cstdarg>

namespace
{
	class FactoryContext : public Resource::IFactoryContext
	{
	public:
		FactoryContext() {}
		virtual ~FactoryContext() {}
	};

	struct TestResourceData
	{
		char internalData_[1024];
	};

	class TestResource
	{
	public:
		DECLARE_RESOURCE(TestResource, 0);

	private:
		friend class TestResourceFactory;

		TestResource() {}
		~TestResource() {}

		TestResourceData* data_ = nullptr;
	};

	class TestResourceFactory : public Resource::IFactory
	{
	public:
		TestResourceFactory() {}
		virtual ~TestResourceFactory() {}

		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			// Check type.
			if(type != TestResource::GetTypeUUID())
				return false;

			TestResource* testResource = new TestResource;
			*outResource = testResource;

			return true;
		}

		bool LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
		    const char* name, Core::File& inFile) override
		{
			// Check type.
			if(type != TestResource::GetTypeUUID())
				return false;

			// Check file validity.
			if(!inFile)
				return false;

			auto* testResource = reinterpret_cast<TestResource*>(*inResource);

			// Check resource if not loaded.
			if(testResource->data_)
				return false;

			// Create resource from the file.
			testResource->data_ = new TestResourceData;
			memset(testResource->data_, 0, sizeof(TestResourceData));
			inFile.Read(testResource->data_->internalData_,
			    Core::Min(inFile.Size(), sizeof(testResource->data_->internalData_)));

			return true;
		}

		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override
		{
			// Check type.
			if(type != TestResource::GetTypeUUID())
				return false;

			// Check resource.
			if(inResource == nullptr || *inResource == nullptr)
				return false;

			auto* typedInResource = reinterpret_cast<TestResource*>(*inResource);

			delete typedInResource->data_;
			delete typedInResource;

			*inResource = nullptr;
			return true;
		}
	};

	DEFINE_RESOURCE(TestResource);

	class ConverterContext : public Resource::IConverterContext
	{
	public:
		ConverterContext() {}

		virtual ~ConverterContext() {}

		void AddDependency(const char* fileName) override { Core::Log("AddDependency: %s\n", fileName); }
		void AddResourceDependency(const char* fileName, const Core::UUID& type) override
		{
			Core::Log("AddResourceDependency: %s\n", fileName);
		}

		void AddOutput(const char* fileName) override { Core::Log("AddOutput: %s\n", fileName); }

		void AddError(const char* errorFile, int errorLine, const char* errorMsg, ...) override
		{
			if(errorFile)
			{
				Core::Log("%s(%d): ", errorFile, errorLine);
			}

			va_list args;
			va_start(args, errorMsg);
			Core::Log(errorMsg, args);
			va_end(args);

			Core::Log("\n");
		}

		Core::IFilePathResolver* GetPathResolver() override { return nullptr; }

		void SetMetaData(MetaDataCb callback, void* metaData) override {}

		void GetMetaData(MetaDataCb callback, void* metaData) override {}
	};
}


TEST_CASE("resource-tests-manager")
{
	Job::Manager::Scoped jobManager(1, 256, 32 * 1024);
	Plugin::Manager::Scoped pluginManager;
	Resource::Manager::Scoped manager;
}


TEST_CASE("resource-tests-request")
{
	Job::Manager::Scoped jobManager(1, 256, 32 * 1024);
	Plugin::Manager::Scoped pluginManager;
	Resource::Manager::Scoped manager;

	// Register factory.
	auto* factory = new TestResourceFactory();
	REQUIRE(Resource::Manager::RegisterFactory(TestResource::GetTypeUUID(), factory));

	{
		auto file = Core::File("converter.test", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		REQUIRE(file);
	}

	TestResource* testResource = nullptr;
	REQUIRE(Resource::Manager::RequestResource(testResource, "converter.test"));
	REQUIRE(testResource);
	REQUIRE(Resource::Manager::ReleaseResource(testResource));
	REQUIRE(!testResource);

	REQUIRE(Resource::Manager::UnregisterFactory(factory));
}


TEST_CASE("resource-tests-request-multiple")
{
	Job::Manager::Scoped jobManager(1, 256, 32 * 1024);
	Plugin::Manager::Scoped pluginManager;
	Resource::Manager::Scoped manager;

	// Register factory.
	auto* factory = new TestResourceFactory();
	REQUIRE(Resource::Manager::RegisterFactory(TestResource::GetTypeUUID(), factory));

	TestResource* testResourceA = nullptr;
	TestResource* testResourceB = nullptr;

	{
		auto file = Core::File("converter.test", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		REQUIRE(file);
	}

	REQUIRE(Resource::Manager::RequestResource(testResourceA, "converter.test"));
	REQUIRE(testResourceA);
	REQUIRE(Resource::Manager::RequestResource(testResourceB, "converter.test"));
	REQUIRE(testResourceB);

	REQUIRE(testResourceA == testResourceB);

	REQUIRE(Resource::Manager::ReleaseResource(testResourceA));
	REQUIRE(!testResourceA);

	Resource::Manager::WaitForResource(testResourceB);

	REQUIRE(Resource::Manager::ReleaseResource(testResourceB));
	REQUIRE(!testResourceB);

	REQUIRE(Resource::Manager::UnregisterFactory(factory));
}


TEST_CASE("resource-tests-refs")
{
	Job::Manager::Scoped jobManager(1, 256, 32 * 1024);
	Plugin::Manager::Scoped pluginManager;
	Resource::Manager::Scoped manager;

	// Register factory.
	auto* factory = new TestResourceFactory();
	REQUIRE(Resource::Manager::RegisterFactory(TestResource::GetTypeUUID(), factory));

	{
		auto file = Core::File("converter.test", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		REQUIRE(file);
	}

	using TestResourceRef = Resource::Ref<TestResource>;

	TestResourceRef testResource("converter.test");
	REQUIRE(testResource);

	testResource.Reset();
	REQUIRE(!testResource);

	REQUIRE(Resource::Manager::UnregisterFactory(factory));
}
