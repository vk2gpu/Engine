#pragma once

#include "core/types.h"
#include "resource/dll.h"

#include "core/uuid.h"

namespace Resource
{

/**
 * Used to declare a resource type.
 * When defining a resource, it should be setup as follows:
 *
 * class Model
 * {
 * public:
 *   DECLARE_RESOURCE("Scene.Model", 1);
 * };
 *
 * This will provide methods used by the templated interfaces
 * on Resource::Manager.
 * Version will be used by the loader to validate the resource.
 */
#define DECLARE_RESOURCE(NAME, VERSION)                                                                                \
	static const char* GetTypeName() { return #NAME; }                                                                 \
	static Core::UUID GetTypeUUID();                                                                                   \
	static const u32 RESOURCE_VERSION = VERSION;                                                                       \
	static void RegisterFactory();                                                                                     \
	static void UnregisterFactory();


/**
 * Used to define a resource in the cpp.
 * Should be inside the namespace of the resource, as follows:
 *
 * namespace Graphics
 * {
 *	DEFINE_RESOURCE(Model);
 * }
 *
 */
#define DEFINE_RESOURCE(CLASS_NAME)                                                                                    \
	static CLASS_NAME##Factory* factory_ = nullptr;                                                                    \
	void CLASS_NAME::RegisterFactory()                                                                                 \
	{                                                                                                                  \
		DBG_ASSERT(factory_ == nullptr);                                                                               \
		factory_ = new CLASS_NAME##Factory();                                                                          \
		Resource::Manager::RegisterFactory<CLASS_NAME>(factory_);                                                      \
	}                                                                                                                  \
	void CLASS_NAME::UnregisterFactory()                                                                               \
	{                                                                                                                  \
		DBG_ASSERT(factory_ != nullptr);                                                                               \
		Resource::Manager::UnregisterFactory(factory_);                                                                \
		delete factory_;                                                                                               \
		factory_ = nullptr;                                                                                            \
	}                                                                                                                  \
	Core::UUID CLASS_NAME::GetTypeUUID()                                                                               \
	{                                                                                                                  \
		static auto uuid = Core::UUID(GetTypeName());                                                                  \
		return uuid;                                                                                                   \
	}

} // namespa e Resource
