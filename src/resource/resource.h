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
 *   DECLARE_RESOURCE(Scene.Model, 1);
 * };
 *
 * This will provide methods used by the templated interfaces
 * on Resource::Manager.
 * Version will be used by the loader to validate the resource.
 */
#define DECLARE_RESOURCE(NAME, VERSION)                                                                                \
	static const char* GetTypeName() { return #NAME; }                                                                 \
	static Core::UUID GetTypeUUID() { return Core::UUID(GetTypeName()); }                                              \
	static const u32 RESOURCE_VERSION = VERSION
}
