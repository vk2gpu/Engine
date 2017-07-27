#pragma once

#include "graphics/dll.h"
#include "graphics/fwd_decls.h"
#include "core/array.h"
#include "gpu/fwd_decls.h"
#include "gpu/types.h"
#include "resource/resource.h"

namespace Graphics
{
	class GRAPHICS_DLL Model
	{
	public:
		DECLARE_RESOURCE("Graphics.Model", 0);

		/// @return Is model ready for use?
		bool IsReady() const { return !!impl_; }

	private:
		friend class ModelFactory;

		Model();
		~Model();
		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		struct ModelImpl* impl_ = nullptr;
	};

} // namespace Graphics
