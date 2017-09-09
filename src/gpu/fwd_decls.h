#pragma once

#include "gpu/dll.h"
#include "core/types.h"

namespace GPU
{
	// command_list.h
	class CommandList;

	// resources.h
	enum class ResourceType : i32;
	class GPU_DLL Handle;
	struct GPU_DLL SwapChainDesc;
	struct GPU_DLL BufferDesc;
	struct GPU_DLL TextureDesc;
	struct GPU_DLL TextureSubResourceData;
	struct GPU_DLL SamplerState;
	struct GPU_DLL ShaderDesc;
	struct GPU_DLL BlendState;
	struct GPU_DLL StencilFaceState;
	struct GPU_DLL RenderState;
	struct GPU_DLL GraphicsPipelineStateDesc;
	struct GPU_DLL ComputePipelineStateDesc;
	struct GPU_DLL BindingRTV;
	struct GPU_DLL BindingDSV;
	struct GPU_DLL BindingSRV;
	struct GPU_DLL BindingUAV;
	struct GPU_DLL BindingBuffer;
	struct GPU_DLL BindingSampler;
	struct GPU_DLL PipelineBindingSetDesc;
	struct GPU_DLL DrawBindingSetDesc;
	struct GPU_DLL FrameBindingSetDesc;

	// types.h
	enum class ErrorCode : i32;
	enum class DebugFlags : u32;
	enum class Format : i32;
	enum class BindFlags : u32;
	enum class TextureType : i32;
	enum class ViewDimension : i32;
	enum class ShaderType : i32;
	enum class TopologyType : i32;
	enum class PrimitiveTopology : i32;
	enum class VertexUsage : i32;
	enum class AddressingMode : i32;
	enum class FilteringMode : i32;
	enum class FillMode : i32;
	enum class CullMode : i32;
	enum class BlendType : i32;
	enum class BlendFunc : i32;
	enum class CompareMode : i32;
	enum class StencilFunc : i32;
	enum class DSVFlags : u32;
	struct GPU_DLL SetupParams;
	struct GPU_DLL AdapterInfo;
	struct GPU_DLL Box;
	struct GPU_DLL Point;
	struct GPU_DLL VertexElement;
	struct GPU_DLL ScissorRect;
	struct GPU_DLL Viewport;
	struct GPU_DLL DrawState;

} // namespace GPU
