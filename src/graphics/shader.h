#pragma once

#include "graphics/dll.h"
#include "graphics/fwd_decls.h"
#include "core/array.h"
#include "gpu/fwd_decls.h"
#include "gpu/types.h"
#include "resource/resource.h"

namespace Graphics
{
	class GRAPHICS_DLL Shader
	{
	public:
		DECLARE_RESOURCE("Graphics.Shader", 0);

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

		/**
		 * @return Binding index. -1 if it doesn't exist.
		 */
		i32 GetBindingIndex(const char* name) const;

		/**
		 * Will create a technique for use during rendering.
		 * If required it'll create pipeline states to match @a desc.
		 */
		ShaderTechnique CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

	private:
		friend class ShaderFactory;

		Shader();
		~Shader();
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		struct ShaderImpl* impl_ = nullptr;
	};

	/**
	 * Descriptor to provide additional information to prepare a technique to be used for rendering.
	 */
	struct GRAPHICS_DLL ShaderTechniqueDesc
	{
		ShaderTechniqueDesc() = default;

		ShaderTechniqueDesc& SetVertexElement(i32 idx, const GPU::VertexElement& element);
		ShaderTechniqueDesc& SetTopology(GPU::TopologyType topology);
		ShaderTechniqueDesc& SetRTVFormat(i32 idx, GPU::Format format);
		ShaderTechniqueDesc& SetDSVFormat(GPU::Format format);

		i32 numVertexElements_ = 0;
		Core::Array<GPU::VertexElement, GPU::MAX_VERTEX_ELEMENTS> vertexElements_ = {};
		GPU::TopologyType topology_ = GPU::TopologyType::INVALID;
		i32 numRTs_ = 0;
		Core::Array<GPU::Format, GPU::MAX_BOUND_RTVS> rtvFormats_ = {};
		GPU::Format dsvFormat_ = GPU::Format::INVALID;
	};

	class GRAPHICS_DLL ShaderTechnique
	{
	public:
		ShaderTechnique();
		~ShaderTechnique();
		ShaderTechnique(ShaderTechnique&&);
		ShaderTechnique& operator=(ShaderTechnique&&);

		// CBV
		void SetCBV(i32 idx, GPU::Handle res, i32 offset, i32 size);

		// Sampler
		void SetSampler(i32 idx, GPU::Handle res);

		// SRV
		void SetBuffer(
		    i32 idx, GPU::Handle res, i32 firstElement = 0, i32 numElements = 0, i32 structureByteStride = 0);
		void SetTexture1D(
		    i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		void SetTexture1DArray(i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0,
		    i32 firstArraySlice = 0, i32 arraySize = 0, f32 resourceMinLODClamp = 0.0f);
		void SetTexture2D(i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 planeSlice = 0,
		    f32 resourceMinLODClamp = 0.0f);
		void SetTexture2DArray(i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0,
		    i32 firstArraySlice = 0, i32 arraySize = 0, i32 planeSlice = 0, f32 resourceMinLODClamp = 0.0f);
		void SetTexture3D(
		    i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		void SetTextureCube(
		    i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		void SetTextureCubeArray(i32 idx, GPU::Handle res, i32 mostDetailedMip = 0, i32 mipLevels = 0,
		    i32 first2DArrayFace = 0, i32 numCubes = 0, f32 resourceMinLODClamp = 0.0f);

		// UAV
		void SetRWBuffer(
		    i32 idx, GPU::Handle res, i32 firstElement = 0, i32 numElements = 0, i32 structureByteStride = 0);
		void SetRWTexture1D(i32 idx, GPU::Handle res, i32 mipSlice = 0);
		void SetRWTexture1DArray(
		    i32 idx, GPU::Handle res, i32 mipSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0);
		void SetRWTexture2D(i32 idx, GPU::Handle res, i32 mipSlice = 0, i32 planeSlice = 0);
		void SetRWTexture2DArray(
		    i32 idx, GPU::Handle res, i32 mipSlice = 0, i32 planeSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0);
		void SetRWTexture3D(i32 idx, GPU::Handle res, i32 mipSlice = 0, i32 firstWSlice = 0, i32 wSize = 0);

		/**
		 * Get binding for the current technique setup.
		 */
		GPU::Handle GetBinding();

		/// @return Is shader technique is valid.
		operator bool() const { return !!impl_; }

	private:
		friend class Shader;

		ShaderTechnique(const ShaderTechnique&) = delete;
		ShaderTechnique& operator=(const ShaderTechnique&) = delete;

		struct ShaderTechniqueImpl* impl_ = nullptr;
	};

} // namespace Graphics
