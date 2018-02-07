#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_resources.h"
#include "gpu/fwd_decls.h"
#include "gpu/resources.h"
#include "core/map.h"

namespace GPU
{
	struct D3D12CompileContext
	{
		D3D12CompileContext(class D3D12Backend& backend);
		~D3D12CompileContext();

		class D3D12Backend& backend_;
		ID3D12GraphicsCommandList* d3dCommandList_ = nullptr;

		struct Subresource
		{
			Subresource() = default;
			Subresource(const D3D12Resource* resource, i32 idx)
			    : resource_(resource)
			    , idx_(idx)
			{
			}

			bool operator==(const Subresource& other) const
			{
				return resource_ == other.resource_ && idx_ == other.idx_;
			}

			const D3D12Resource* resource_ = nullptr;
			i32 idx_ = 0;
		};

		struct SubresourceHasher
		{
			u32 operator()(u32 input, const Subresource& data) const
			{
				input = Core::HashCRC32(input, &data.resource_, sizeof(data.resource_));
				return Core::HashCRC32(input, &data.idx_, sizeof(data.idx_));
			}
		};

		Core::Map<Subresource, D3D12_RESOURCE_STATES, SubresourceHasher> stateTracker_;
		Core::Map<Subresource, D3D12_RESOURCE_BARRIER, SubresourceHasher> pendingBarriers_;
		Core::Vector<D3D12_RESOURCE_BARRIER> barriers_;

		struct DescriptorCopyParams
		{
			Core::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> dstHandles_;
			Core::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcHandles_;
			Core::Vector<UINT> numHandles_;
		};

		DescriptorCopyParams samplerDescCopyParams_;
		DescriptorCopyParams viewDescCopyParams_;

		DrawState drawState_;
		DrawState* cachedDrawState_ = nullptr;
		Viewport cachedViewport_;
		ScissorRect cachedScissorRect_;
		u8 cachedStencilRef_ = 0;

		GPU::Handle dbsBound_;
		PrimitiveTopology primitiveBound_ = PrimitiveTopology::INVALID;
		GPU::Handle fbsBound_;
		RootSignatureType rootSigBound_ = RootSignatureType::INVALID;
		ID3D12PipelineState* psBound_ = nullptr;

		Core::Array<ID3D12DescriptorHeap*, 2> descHeapsBound_ = {};
		Core::Array<D3D12_GPU_DESCRIPTOR_HANDLE, 8> gfxDescHandlesBound_ = {};
		Core::Array<D3D12_GPU_DESCRIPTOR_HANDLE, 8> compDescHandlesBound_ = {};

		Core::Vector<const char*> eventStack_;

		ErrorCode CompileCommandList(class D3D12CommandList& outCommandList, const class CommandList& commandList);
		ErrorCode CompileCommand(const struct CommandDraw* command);
		ErrorCode CompileCommand(const struct CommandDrawIndirect* command);
		ErrorCode CompileCommand(const struct CommandDispatch* command);
		ErrorCode CompileCommand(const struct CommandDispatchIndirect* command);
		ErrorCode CompileCommand(const struct CommandClearRTV* command);
		ErrorCode CompileCommand(const struct CommandClearDSV* command);
		ErrorCode CompileCommand(const struct CommandClearUAV* command);
		ErrorCode CompileCommand(const struct CommandUpdateBuffer* command);
		ErrorCode CompileCommand(const struct CommandUpdateTextureSubResource* command);
		ErrorCode CompileCommand(const struct CommandCopyBuffer* command);
		ErrorCode CompileCommand(const struct CommandCopyTextureSubResource* command);

		ErrorCode SetDrawBinding(Handle dbsHandle, PrimitiveTopology primitive);
		ErrorCode SetPipeline(Handle ps, Core::ArrayView<PipelineBinding> pbs);
		ErrorCode SetFrameBinding(Handle fbsHandle);
		ErrorCode SetDrawState(const DrawState* drawState);

		/**
		 * Add resource transition.
		 * @param subRsc Subresource range.
		 * @param state States to transition.
		 * @return Has any state changed?
		 */
		bool AddTransition(const D3D12SubresourceRange& subRsc, D3D12_RESOURCE_STATES state);

		/**
		 * Add resource transition.
		 * @param resource Resource to transition.
		 * @param firstSubRsc First subresource.
		 * @param numSubRsc Number of subresources to transition.
		 * @param state States to transition.
		 * @return Has any state changed?
		 */
		bool AddTransition(const D3D12Resource* resource, i32 firstSubRsc, i32 numSubRsc, D3D12_RESOURCE_STATES state);


		/**
		 * Add UAV barrier.
		 * @param subRsc Subresource range to add appropriate barriers for.
		 */
		void AddUAVBarrier(const D3D12SubresourceRange& subRsc);

		/**
		 * Flush resource transitions.
		 */
		void FlushTransitions();

		/**
		 * Flush descriptors.
		 */
		void FlushDescriptors();

		/**
		 * Restore default state of resources.
		 */
		void RestoreDefault();
	};
} // namespace GPU
