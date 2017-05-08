#pragma once

#include "core/types.h"
#include "gpu/fwd_decls.h"
#include "graphics/converters/image.h"
namespace Resource
{
	class IConverterContext;
} // namespace Resource;

namespace Graphics
{
	namespace DDS
	{
		Graphics::Image LoadImage(Resource::IConverterContext& context, const char* sourceFile);
	} // namespace DDS
} // namespace Graphics
