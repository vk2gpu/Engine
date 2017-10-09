#pragma once

#include "serialization/serializer.h"

namespace Graphics
{
	struct ImportMaterial
	{
		Core::String shader_;
		Core::Map<Core::String, Core::String> textures_;

		bool Serialize(Serialization::Serializer& serializer)
		{
			serializer.Serialize("shader", shader_);
			serializer.Serialize("textures", textures_);
			return true;
		}
	};

	struct MetaDataMaterial
	{
		bool isInitialized_ = false;
		bool Serialize(Serialization::Serializer& serializer)
		{
			isInitialized_ = true;
			return true;
		}
	};
} // namespace Graphics
