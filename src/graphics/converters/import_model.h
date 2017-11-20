#pragma once

#include "gpu/types.h"
#include "serialization/serializer.h"

#include "import_material.h"

namespace Graphics
{
	struct MetaDataModel
	{
		bool isInitialized_ = false;
		bool flattenHierarchy_ = false;
		bool splitStreams_ = true;
		i32 maxBones_ = 256;
		i32 maxBoneInfluences_ = 4;
		f32 smoothingAngle_ = 90.0f;

		struct
		{
			GPU::Format position_ = GPU::Format::R32G32B32_FLOAT;
			GPU::Format normal_ = GPU::Format::R8G8B8A8_SNORM;
			GPU::Format tangent_ = GPU::Format::R8G8B8A8_SNORM;
			GPU::Format texcoord_ = GPU::Format::R16G16_FLOAT;
			GPU::Format color_ = GPU::Format::R8G8B8A8_UNORM;
		} vertexFormat_;

		struct Material
		{
			Core::String regex_;
			ImportMaterial template_;

			bool Serialize(Serialization::Serializer& serializer)
			{
				bool retVal = true;
				retVal &= serializer.Serialize("regex", regex_);
				retVal &= serializer.Serialize("template", template_);
				return retVal;
			}
		};

		Core::Vector<Material> materials_;

		bool Serialize(Serialization::Serializer& serializer)
		{
			isInitialized_ = true;

			bool retVal = true;
			retVal &= serializer.Serialize("flattenHierarchy", flattenHierarchy_);
			retVal &= serializer.Serialize("splitStreams", splitStreams_);
			retVal &= serializer.Serialize("smoothingAngle", smoothingAngle_);
			retVal &= serializer.Serialize("maxBones", maxBones_);
			retVal &= serializer.Serialize("maxBoneInfluences", maxBoneInfluences_);
			if(auto object = serializer.Object("vertexFormat"))
			{
				retVal &= serializer.Serialize("position", vertexFormat_.position_);
				retVal &= serializer.Serialize("normal", vertexFormat_.normal_);
				retVal &= serializer.Serialize("tangent", vertexFormat_.tangent_);
				retVal &= serializer.Serialize("texcoord", vertexFormat_.texcoord_);
				retVal &= serializer.Serialize("color", vertexFormat_.color_);
			}
			retVal &= serializer.Serialize("materials", materials_);
			return retVal;
		}
	};
} // namespace Graphics
