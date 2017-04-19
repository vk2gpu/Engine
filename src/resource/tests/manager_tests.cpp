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
#include "resource/resource.h"

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
		friend class TestFactory;

		TestResource() {}
		~TestResource() {}

		TestResourceData* data_ = nullptr;
	};

	class TestFactory : public Resource::IFactory
	{
	public:
		TestFactory() {}
		virtual ~TestFactory() {}

		bool CreateResource(
			Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			// Check type.
			if(type != TestResource::GetTypeUUID())
				return false;

			TestResource* testResource = new TestResource;
			*outResource = testResource;

			return true;
		}

		bool LoadResource(
		    Resource::IFactoryContext& context, void** inResource, const Core::UUID& type, Core::File& inFile) override
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

	class ConverterContext : public Resource::IConverterContext
	{
	public:
		ConverterContext() {}

		virtual ~ConverterContext() {}

		void AddDependency(const char* fileName) override { Core::Log("AddDependency: %s\n", fileName); }

		void AddOutput(const char* fileName) override { Core::Log("AddOutput: %s\n", fileName); }

		void AddError(const char* errorFile, int errorLine, const char* errorMsg) override
		{
			if(errorFile)
			{
				Core::Log("%s(%d): %s\n", errorFile, errorLine, errorMsg);
			}
			else
			{
				Core::Log("%s\n", errorMsg);
			}
		}
	};
}


TEST_CASE("resource-tests-manager")
{
	Job::Manager jobManager(1, 256, 32 * 1024);
	Plugin::Manager pluginManager;
	Resource::Manager manager(jobManager, pluginManager);
}


TEST_CASE("resource-tests-request")
{
	Job::Manager jobManager(1, 256, 32 * 1024);
	Plugin::Manager pluginManager;
	Resource::Manager manager(jobManager, pluginManager);

	// Register factory.
	auto* factory = new TestFactory();
	REQUIRE(manager.RegisterFactory(TestResource::GetTypeUUID(), factory));

	TestResource* testResource = nullptr;
	REQUIRE(manager.RequestResource(testResource, "converter.test"));
	REQUIRE(testResource);
	REQUIRE(manager.ReleaseResource(testResource));
	REQUIRE(!testResource);

	REQUIRE(manager.UnregisterFactory(factory));
}

TEST_CASE("resource-tests-request-multiple")
{
	Job::Manager jobManager(1, 256, 32 * 1024);
	Plugin::Manager pluginManager;
	Resource::Manager manager(jobManager, pluginManager);

	// Register factory.
	auto* factory = new TestFactory();
	REQUIRE(manager.RegisterFactory(TestResource::GetTypeUUID(), factory));

	TestResource* testResourceA = nullptr;
	TestResource* testResourceB = nullptr;

	REQUIRE(manager.RequestResource(testResourceA, "converter.test"));
	REQUIRE(testResourceA);
	REQUIRE(manager.RequestResource(testResourceB, "converter.test"));
	REQUIRE(testResourceB);

	REQUIRE(testResourceA == testResourceB);

	REQUIRE(manager.ReleaseResource(testResourceA));
	REQUIRE(!testResourceA);

	manager.WaitForResource(testResourceB);

	REQUIRE(manager.ReleaseResource(testResourceB));
	REQUIRE(!testResourceB);

	REQUIRE(manager.UnregisterFactory(factory));
}
