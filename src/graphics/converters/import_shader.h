#pragma once

#include "serialization/serializer.h"

namespace Graphics
{
	struct MetaDataShader
	{
		bool isInitialized_ = false;

		bool Serialize(Serialization::Serializer& serializer)
		{
			isInitialized_ = true;
			return true;
		}
	};
} // namespace Graphics
