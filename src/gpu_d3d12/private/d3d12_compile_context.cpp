#include "gpu_d3d12/d3d12_compile_context.h"
#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_backend.h"
#include "gpu_d3d12/d3d12_device.h"
#include "gpu_d3d12/d3d12_linear_descriptor_allocator.h"
#include "gpu_d3d12/d3d12_linear_heap_allocator.h"
#include "gpu/command_list.h"
#include "gpu/commands.h"
#include "gpu/manager.h"

#include <pix_win.h>

#define DEBUG_TRANSITIONS (0)

namespace GPU
{
	D3D12CompileContext::D3D12CompileContext(D3D12Backend& backend)
	    : backend_(backend)
	{
	}

	D3D12CompileContext::~D3D12CompileContext() {}

	ErrorCode D3D12CompileContext::CompileCommandList(D3D12CommandList& outCommandList, const CommandList& commandList)
	{
		d3dCommandList_ = outCommandList.Open();
		bool supportPIXMarkers = true;
		bool supportAGSMarkers = false;
		auto device = backend_.device_;

		if(backend_.agsContext_ && Core::ContainsAllFlags(device->agsFeatureBits_, AGS_DX12_EXTENSION_USER_MARKERS))
			supportAGSMarkers = true;

		if(d3dCommandList_)
		{
#define CASE_COMMAND(TYPE_STRUCT)                                                                                      \
	case TYPE_STRUCT::TYPE:                                                                                            \
		RETURN_ON_ERROR(CompileCommand(static_cast<const TYPE_STRUCT*>(command)));                                     \
		break

			for(const auto* command : commandList)
			{
				switch(command->type_)
				{
					CASE_COMMAND(CommandDraw);
					CASE_COMMAND(CommandDrawIndirect);
					CASE_COMMAND(CommandDispatch);
					CASE_COMMAND(CommandDispatchIndirect);
					CASE_COMMAND(CommandClearRTV);
					CASE_COMMAND(CommandClearDSV);
					CASE_COMMAND(CommandClearUAV);
					CASE_COMMAND(CommandUpdateBuffer);
					CASE_COMMAND(CommandUpdateTextureSubResource);
					CASE_COMMAND(CommandCopyBuffer);
					CASE_COMMAND(CommandCopyTextureSubResource);
					break;

				// Debug events.
				case CommandBeginEvent::TYPE:
				{
					const auto* eventCommand = static_cast<const CommandBeginEvent*>(command);

					if(supportPIXMarkers)
						PIXBeginEvent(d3dCommandList_, eventCommand->metaData_, eventCommand->text_);
					if(supportAGSMarkers)
						agsDriverExtensionsDX12_PushMarker(backend_.agsContext_, d3dCommandList_, eventCommand->text_);
				}
				break;
				case CommandEndEvent::TYPE:
				{
					if(supportPIXMarkers)
						PIXEndEvent(d3dCommandList_);
					if(supportAGSMarkers)
						agsDriverExtensionsDX12_PopMarker(backend_.agsContext_, d3dCommandList_);
				}
				break;
				default:
					DBG_ASSERT(false);
				}
			}

#undef CASE_COMMAND

			//FlushDescriptors();
			RestoreDefault();
			return outCommandList.Close();
		}
		return ErrorCode::FAIL;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDraw* command)
	{
		SetPipeline(command->pipelineState_, command->pipelineBindings_);
		SetFrameBinding(command->frameBinding_);
		SetDrawState(command->drawState_);

		if(command->drawBinding_ != GPU::Handle())
		{
			const auto& dbs = backend_.drawBindingSets_[command->drawBinding_.GetIndex()];

			SetDrawBinding(command->drawBinding_, command->primitive_);

			FlushTransitions();
			if(dbs.ib_.BufferLocation == 0)
			{
				d3dCommandList_->DrawInstanced(
				    command->noofVertices_, command->noofInstances_, command->vertexOffset_, command->firstInstance_);
			}
			else
			{
				d3dCommandList_->DrawIndexedInstanced(command->noofVertices_, command->noofInstances_,
				    command->indexOffset_, command->vertexOffset_, command->firstInstance_);
			}
		}
		else
		{
			d3dCommandList_->IASetPrimitiveTopology(GetPrimitiveTopology(command->primitive_));

			FlushTransitions();
			d3dCommandList_->DrawInstanced(
			    command->noofVertices_, command->noofInstances_, command->vertexOffset_, command->firstInstance_);
		}
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDrawIndirect* command)
	{
		const auto& indirectBuffer = backend_.bufferResources_[command->indirectBuffer_.GetIndex()];
		const auto* countBuffer =
		    (command->countBuffer_) ? &backend_.bufferResources_[command->countBuffer_.GetIndex()] : nullptr;

		SetPipeline(command->pipelineState_, command->pipelineBindings_);
		SetFrameBinding(command->frameBinding_);
		SetDrawState(command->drawState_);
		AddTransition(&indirectBuffer, 0, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		if(countBuffer)
			AddTransition(countBuffer, 0, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		if(command->drawBinding_ != GPU::Handle())
		{
			const auto& dbs = backend_.drawBindingSets_[command->drawBinding_.GetIndex()];

			SetDrawBinding(command->drawBinding_, command->primitive_);

			FlushTransitions();
			if(dbs.ib_.BufferLocation == 0)
			{
				d3dCommandList_->ExecuteIndirect(backend_.device_->d3dDrawCmdSig_.Get(), command->maxCommands_,
				    indirectBuffer.resource_.Get(), command->argByteOffset_, countBuffer->resource_.Get(),
				    command->countByteOffset_);
			}
			else
			{
				d3dCommandList_->ExecuteIndirect(backend_.device_->d3dDrawIndexedCmdSig_.Get(), command->maxCommands_,
				    indirectBuffer.resource_.Get(), command->argByteOffset_, countBuffer->resource_.Get(),
				    command->countByteOffset_);
			}
		}
		else
		{
			d3dCommandList_->IASetPrimitiveTopology(GetPrimitiveTopology(command->primitive_));

			FlushTransitions();
			d3dCommandList_->ExecuteIndirect(backend_.device_->d3dDrawCmdSig_.Get(), command->maxCommands_,
			    indirectBuffer.resource_.Get(), command->argByteOffset_, countBuffer->resource_.Get(),
			    command->countByteOffset_);
		}
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDispatch* command)
	{
		SetPipeline(command->pipelineState_, command->pipelineBindings_);

		FlushTransitions();
		d3dCommandList_->Dispatch(command->xGroups_, command->yGroups_, command->zGroups_);
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDispatchIndirect* command)
	{
		const auto& indirectBuffer = backend_.bufferResources_[command->indirectBuffer_.GetIndex()];
		const auto* countBuffer =
		    (command->countBuffer_) ? &backend_.bufferResources_[command->countBuffer_.GetIndex()] : nullptr;

		SetPipeline(command->pipelineState_, command->pipelineBindings_);
		AddTransition(&indirectBuffer, 0, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		if(countBuffer)
			AddTransition(countBuffer, 0, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		FlushTransitions();
		d3dCommandList_->ExecuteIndirect(backend_.device_->d3dDispatchCmdSig_.Get(), command->maxCommands_,
		    indirectBuffer.resource_.Get(), command->argByteOffset_, countBuffer->resource_.Get(),
		    command->countByteOffset_);
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearRTV* command)
	{
		const auto& fbs = backend_.frameBindingSets_[command->frameBinding_.GetIndex()];
		DBG_ASSERT(command->rtvIdx_ < fbs.numRTs_);


		D3D12_CPU_DESCRIPTOR_HANDLE handle = fbs.rtvs_.cpuDescHandle_;
		i32 rtvIdx =
		    fbs.swapChain_ == nullptr ? command->rtvIdx_ : command->rtvIdx_ + fbs.swapChain_->bbIdx_ * MAX_BOUND_RTVS;

		handle.ptr +=
		    rtvIdx * backend_.device_->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto& subRsc = fbs.rtvResources_[rtvIdx];
		AddTransition(subRsc, D3D12_RESOURCE_STATE_RENDER_TARGET);
		FlushTransitions();

		d3dCommandList_->ClearRenderTargetView(handle, command->color_, 0, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearDSV* command)
	{
		const auto& fbs = backend_.frameBindingSets_[command->frameBinding_.GetIndex()];
		DBG_ASSERT(fbs.desc_.dsv_.resource_);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = fbs.dsv_.cpuDescHandle_;

		auto& subRsc = fbs.dsvResource_;
		AddTransition(subRsc, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		FlushTransitions();

		d3dCommandList_->ClearDepthStencilView(
		    handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, command->depth_, command->stencil_, 0, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearUAV* command)
	{
		const auto& pbs = backend_.pipelineBindingSets_[command->pipelineBinding_.GetIndex()];

		auto incSize =
		    backend_.device_->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = pbs.uavs_.gpuDescHandle_;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pbs.uavs_.cpuDescHandle_;
		gpuHandle.ptr += incSize * command->uavIdx_;
		cpuHandle.ptr += incSize * command->uavIdx_;

		auto& subRsc = pbs.uavTransitions_[command->uavIdx_];
		AddTransition(subRsc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		FlushTransitions();

		d3dCommandList_->ClearUnorderedAccessViewUint(
		    gpuHandle, cpuHandle, subRsc.resource_->resource_.Get(), command->u_, 0, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandUpdateBuffer* command)
	{
		const auto* buf = backend_.GetD3D12Buffer(command->buffer_);
		DBG_ASSERT(buf && buf->resource_);

		auto uploadAlloc = backend_.device_->GetUploadAllocator().Alloc(command->size_);
		memcpy(uploadAlloc.address_, command->data_, command->size_);

		AddTransition(buf, 0, 1, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushTransitions();

		d3dCommandList_->CopyBufferRegion(buf->resource_.Get(), command->offset_, uploadAlloc.baseResource_.Get(),
		    uploadAlloc.offsetInBaseResource_, command->size_);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandUpdateTextureSubResource* command)
	{
		const auto* tex = backend_.GetD3D12Texture(command->texture_);
		DBG_ASSERT(tex && tex->resource_);

		auto srcLayout = command->data_;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT dstLayout;
		i32 numRows = 0;
		i64 rowSizeInBytes = 0;
		i64 totalBytes = 0;
		D3D12_RESOURCE_DESC resDesc = GetResourceDesc(tex->desc_);

		backend_.device_->d3dDevice_->GetCopyableFootprints(&resDesc, command->subResourceIdx_, 1, 0, &dstLayout,
		    (u32*)&numRows, (u64*)&rowSizeInBytes, (u64*)&totalBytes);

		auto resAlloc = backend_.device_->GetUploadAllocator().Alloc(totalBytes);
		DBG_ASSERT(srcLayout.rowPitch_ <= rowSizeInBytes);
		const u8* srcData = (const u8*)command->data_.data_;
		u8* dstData = (u8*)resAlloc.address_ + dstLayout.Offset;
		for(i32 slice = 0; slice < tex->desc_.depth_; ++slice)
		{
			const u8* rowSrcData = srcData;
			for(i32 row = 0; row < numRows; ++row)
			{
				memcpy(dstData, srcData, srcLayout.rowPitch_);
				dstData += rowSizeInBytes;
				rowSrcData += srcLayout.rowPitch_;
			}
			rowSrcData += srcLayout.slicePitch_;
		}

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = tex->resource_.Get();
		dst.SubresourceIndex = command->subResourceIdx_;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = resAlloc.baseResource_.Get();
		src.PlacedFootprint = dstLayout;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		AddTransition(tex, command->subResourceIdx_, 1, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushTransitions();
		d3dCommandList_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandCopyBuffer* command)
	{
		const auto* dstBuf = backend_.GetD3D12Buffer(command->dstBuffer_);
		const auto* srcBuf = backend_.GetD3D12Buffer(command->srcBuffer_);
		DBG_ASSERT(dstBuf && dstBuf->resource_);
		DBG_ASSERT(srcBuf && srcBuf->resource_);

		AddTransition(dstBuf, 0, 1, D3D12_RESOURCE_STATE_COPY_DEST);
		AddTransition(srcBuf, 0, 1, D3D12_RESOURCE_STATE_COPY_SOURCE);
		FlushTransitions();

		d3dCommandList_->CopyBufferRegion(dstBuf->resource_.Get(), command->dstOffset_, srcBuf->resource_.Get(),
		    command->srcOffset_, command->srcSize_);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandCopyTextureSubResource* command)
	{
		const auto* dstTex = backend_.GetD3D12Texture(command->dstTexture_);
		const auto* srcTex = backend_.GetD3D12Texture(command->srcTexture_);
		DBG_ASSERT(dstTex && dstTex->resource_);
		DBG_ASSERT(srcTex && srcTex->resource_);

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = dstTex->resource_.Get();
		dst.SubresourceIndex = command->dstSubResourceIdx_;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = srcTex->resource_.Get();
		src.SubresourceIndex = command->srcSubResourceIdx_;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		AddTransition(dstTex, dst.SubresourceIndex, 1, D3D12_RESOURCE_STATE_COPY_DEST);
		AddTransition(srcTex, src.SubresourceIndex, 1, D3D12_RESOURCE_STATE_COPY_SOURCE);
		FlushTransitions();

		D3D12_BOX box;
		box.left = command->srcBox_.x_;
		box.top = command->srcBox_.y_;
		box.front = command->srcBox_.z_;
		box.right = command->srcBox_.x_ + command->srcBox_.w_;
		box.bottom = command->srcBox_.y_ + command->srcBox_.h_;
		box.back = command->srcBox_.z_ + command->srcBox_.d_;
		d3dCommandList_->CopyTextureRegion(
		    &dst, command->dstPoint_.x_, command->dstPoint_.y_, command->dstPoint_.z_, &src, &box);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetDrawBinding(Handle dbsHandle, PrimitiveTopology primitive)
	{
		if(dbsBound_ != dbsHandle)
		{
			dbsBound_ = dbsHandle;

			const auto& dbs = backend_.drawBindingSets_[dbsHandle.GetIndex()];

			// Setup draw binding.
			if(dbs.ibResource_)
			{
				AddTransition(dbs.ibResource_, 0, 1, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				d3dCommandList_->IASetIndexBuffer(&dbs.ib_);
			}

			for(i32 i = 0; i < MAX_VERTEX_STREAMS; ++i)
				if(dbs.vbResources_[i])
					AddTransition(dbs.vbResources_[i], 0, 1, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			d3dCommandList_->IASetVertexBuffers(0, MAX_VERTEX_STREAMS, dbs.vbs_.data());
		}

		if(primitiveBound_ != primitive)
		{
			primitiveBound_ = primitive;
			d3dCommandList_->IASetPrimitiveTopology(GetPrimitiveTopology(primitive));
		}
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetPipeline(Handle ps, Core::ArrayView<PipelineBinding> pb)
	{
#if 1
		DBG_ASSERT(pb.size() == 1);
		DBG_ASSERT(pb[0].pbs_.IsValid());
		const auto& pbs = backend_.pipelineBindingSets_[pb[0].pbs_.GetIndex()];

		auto device = backend_.device_;
		const i32 samplerIncrSize =
		    device->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		const i32 viewIncrSize =
		    device->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ID3D12DescriptorHeap* samplerHeap = nullptr;
		ID3D12DescriptorHeap* viewHeap = nullptr;

		// Validate heaps.
		samplerHeap = pbs.samplers_.d3dDescriptorHeap_.Get();
		DBG_ASSERT(samplerHeap);
		DBG_ASSERT(Core::ContainsAllFlags(samplerHeap->GetDesc().Flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));

		viewHeap = pbs.cbvs_.d3dDescriptorHeap_.Get();
		DBG_ASSERT(viewHeap);
		DBG_ASSERT(viewHeap == pbs.srvs_.d3dDescriptorHeap_.Get());
		DBG_ASSERT(viewHeap == pbs.uavs_.d3dDescriptorHeap_.Get());
		DBG_ASSERT(Core::ContainsAllFlags(viewHeap->GetDesc().Flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));

		// Lazily setup transitions.
		// TODO: Some better transition management here.
		// Not all resources nessisarily need transitions.
		for(i32 i = 0; i < pbs.cbvTransitions_.size(); ++i)
		{
			DBG_ASSERT(pbs.cbvTransitions_[i])
			AddTransition(pbs.cbvTransitions_[i], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}

		for(i32 i = 0; i < pbs.srvTransitions_.size(); ++i)
		{
			DBG_ASSERT(pbs.srvTransitions_[i]);
			AddTransition(pbs.srvTransitions_[i],
			    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		for(i32 i = 0; i < pbs.uavTransitions_.size(); ++i)
		{
			DBG_ASSERT(pbs.uavTransitions_[i]);
			AddUAVBarrier(pbs.uavTransitions_[i]);
		}

#else
		auto device = backend_.device_;

		// Setup descriptors.
		auto& samplerAllocator = device->GetSamplerDescriptorAllocator();
		auto& viewAllocator = device->GetViewDescriptorAllocator();
		auto& cbvSubAllocator = device->GetCBVSubAllocator();
		auto& srvSubAllocator = device->GetSRVSubAllocator();
		auto& uavSubAllocator = device->GetUAVSubAllocator();

		i32 samplerIncrSize = samplerAllocator.GetHandleIncrementSize();
		i32 viewIncrSize = viewAllocator.GetHandleIncrementSize();

		auto descSamplers = samplerAllocator.Alloc(MAX_SAMPLER_BINDINGS, GPU::DescriptorHeapSubType::SAMPLER);
		auto descCBVs = cbvSubAllocator.Alloc(MAX_CBV_BINDINGS, MAX_CBV_BINDINGS);
		auto descSRVs = srvSubAllocator.Alloc(MAX_SRV_BINDINGS, MAX_SRV_BINDINGS);
		auto descUAVs = uavSubAllocator.Alloc(MAX_UAV_BINDINGS, MAX_UAV_BINDINGS);

		DBG_ASSERT(descSamplers.size_ > 0);
		DBG_ASSERT(descCBVs.size_ > 0);
		DBG_ASSERT(descSRVs.size_ > 0);
		DBG_ASSERT(descUAVs.size_ > 0);

		for(const auto& thisPb : pb)
		{
			DBG_ASSERT(thisPb.pbs_.IsValid());
			const auto& pbs = backend_.pipelineBindingSets_[thisPb.pbs_.GetIndex()];

			auto OffsetHandle = [](D3D12_CPU_DESCRIPTOR_HANDLE handle, i32 offset, i32 incr) {
				handle.ptr += offset * incr;
				return handle;
			};

			if(pbs.cbvs_.size_ > 0)
			{
				viewDescCopyParams_.dstHandles_.push_back(
				    OffsetHandle(descCBVs.cpuDescHandle_, thisPb.cbvs_.dstOffset_, viewIncrSize));
				viewDescCopyParams_.srcHandles_.push_back(
				    OffsetHandle(pbs.cbvs_.cpuDescHandle_, thisPb.cbvs_.srcOffset_, viewIncrSize));
				viewDescCopyParams_.numHandles_.push_back(pbs.cbvs_.size_);
			}

			if(pbs.srvs_.size_ > 0)
			{
				viewDescCopyParams_.dstHandles_.push_back(
				    OffsetHandle(descSRVs.cpuDescHandle_, thisPb.srvs_.dstOffset_, viewIncrSize));
				viewDescCopyParams_.srcHandles_.push_back(
				    OffsetHandle(pbs.srvs_.cpuDescHandle_, thisPb.srvs_.srcOffset_, viewIncrSize));
				viewDescCopyParams_.numHandles_.push_back(pbs.srvs_.size_);
			}

			if(pbs.uavs_.size_ > 0)
			{
				viewDescCopyParams_.dstHandles_.push_back(
				    OffsetHandle(descUAVs.cpuDescHandle_, thisPb.uavs_.dstOffset_, viewIncrSize));
				viewDescCopyParams_.srcHandles_.push_back(
				    OffsetHandle(pbs.uavs_.cpuDescHandle_, thisPb.uavs_.srcOffset_, viewIncrSize));
				viewDescCopyParams_.numHandles_.push_back(pbs.uavs_.size_);
			}

			if(pbs.samplers_.size_ > 0)
			{
				samplerDescCopyParams_.dstHandles_.push_back(
				    OffsetHandle(descSamplers.cpuDescHandle_, thisPb.samplers_.dstOffset_, samplerIncrSize));
				samplerDescCopyParams_.srcHandles_.push_back(
				    OffsetHandle(pbs.samplers_.cpuDescHandle_, thisPb.samplers_.srcOffset_, samplerIncrSize));
				samplerDescCopyParams_.numHandles_.push_back(pbs.samplers_.size_);
			}

			// Lazily setup transitions.
			// TODO: Some better transition management here.
			// Not all resources nessisarily need transitions.
			for(i32 i = 0; i < pbs.cbvTransitions_.size(); ++i)
				if(pbs.cbvTransitions_[i] && pbs.cbvTransitions_[i].resource_->resource_)
					AddTransition(pbs.cbvTransitions_[i], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			for(i32 i = 0; i < pbs.srvTransitions_.size(); ++i)
				if(pbs.srvTransitions_[i] && pbs.srvTransitions_[i].resource_->resource_)
					AddTransition(pbs.srvTransitions_[i],
					    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			for(i32 i = 0; i < pbs.uavTransitions_.size(); ++i)
				if(pbs.uavTransitions_[i] && pbs.uavTransitions_[i].resource_->resource_)
					AddUAVBarrier(pbs.uavTransitions_[i]);
		}
#endif

		if(descHeapsBound_[0] != viewHeap || descHeapsBound_[1] != samplerHeap)
		{
			descHeapsBound_[0] = viewHeap;
			descHeapsBound_[1] = samplerHeap;

			d3dCommandList_->SetDescriptorHeaps(2, &descHeapsBound_[0]);
		}

		ID3D12PipelineState* d3d12PipelineState = nullptr;
		RootSignatureType rootSig = RootSignatureType::INVALID;
		if(ps.GetType() == ResourceType::COMPUTE_PIPELINE_STATE)
		{
			d3d12PipelineState = backend_.computePipelineStates_[ps.GetIndex()].pipelineState_.Get();
			rootSig = RootSignatureType::COMPUTE;
		}
		if(ps.GetType() == ResourceType::GRAPHICS_PIPELINE_STATE)
		{
			d3d12PipelineState = backend_.graphicsPipelineStates_[ps.GetIndex()].pipelineState_.Get();
			rootSig = RootSignatureType::GRAPHICS;
		}

		if(psBound_ != d3d12PipelineState)
		{
			d3dCommandList_->SetPipelineState(d3d12PipelineState);
			psBound_ = d3d12PipelineState;
		}

		bool rootSigChanged = false;
		switch(rootSig)
		{
		case RootSignatureType::GRAPHICS:
			if(rootSigBound_ != rootSig)
			{
				d3dCommandList_->SetGraphicsRootSignature(backend_.device_->d3dRootSignatures_[(i32)rootSig].Get());
				rootSigBound_ = rootSig;
				rootSigChanged = true;
			}

			if(rootSigChanged || gfxDescHandlesBound_[0].ptr != pbs.samplers_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(
				    pbs.samplers_.gpuDescHandle_.ptr ==
				    (samplerHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.samplers_.offset_ * samplerIncrSize));
				d3dCommandList_->SetGraphicsRootDescriptorTable(0, pbs.samplers_.gpuDescHandle_);
				gfxDescHandlesBound_[0] = pbs.samplers_.gpuDescHandle_;
			}

			if(rootSigChanged || gfxDescHandlesBound_[1].ptr != pbs.cbvs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.cbvs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.cbvs_.offset_ * viewIncrSize));
				d3dCommandList_->SetGraphicsRootDescriptorTable(1, pbs.cbvs_.gpuDescHandle_);
				gfxDescHandlesBound_[1] = pbs.cbvs_.gpuDescHandle_;
			}

			if(rootSigChanged || gfxDescHandlesBound_[2].ptr != pbs.srvs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.srvs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.srvs_.offset_ * viewIncrSize));
				d3dCommandList_->SetGraphicsRootDescriptorTable(2, pbs.srvs_.gpuDescHandle_);
				gfxDescHandlesBound_[2] = pbs.srvs_.gpuDescHandle_;
			}

			if(rootSigChanged || gfxDescHandlesBound_[3].ptr != pbs.uavs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.uavs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.uavs_.offset_ * viewIncrSize));
				d3dCommandList_->SetGraphicsRootDescriptorTable(3, pbs.uavs_.gpuDescHandle_);
				gfxDescHandlesBound_[3] = pbs.uavs_.gpuDescHandle_;
			}

			break;
		case RootSignatureType::COMPUTE:
			if(rootSigBound_ != rootSig)
			{
				d3dCommandList_->SetComputeRootSignature(backend_.device_->d3dRootSignatures_[(i32)rootSig].Get());
				rootSigBound_ = rootSig;
				rootSigChanged = true;
			}

			if(rootSigChanged || compDescHandlesBound_[0].ptr != pbs.samplers_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(
				    pbs.samplers_.gpuDescHandle_.ptr ==
				    (samplerHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.samplers_.offset_ * samplerIncrSize));
				d3dCommandList_->SetComputeRootDescriptorTable(0, pbs.samplers_.gpuDescHandle_);
				compDescHandlesBound_[0] = pbs.samplers_.gpuDescHandle_;
			}

			if(rootSigChanged || compDescHandlesBound_[1].ptr != pbs.cbvs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.cbvs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.cbvs_.offset_ * viewIncrSize));
				d3dCommandList_->SetComputeRootDescriptorTable(1, pbs.cbvs_.gpuDescHandle_);
				compDescHandlesBound_[1] = pbs.cbvs_.gpuDescHandle_;
			}

			if(rootSigChanged || compDescHandlesBound_[2].ptr != pbs.srvs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.srvs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.srvs_.offset_ * viewIncrSize));
				d3dCommandList_->SetComputeRootDescriptorTable(2, pbs.srvs_.gpuDescHandle_);
				compDescHandlesBound_[2] = pbs.srvs_.gpuDescHandle_;
			}

			if(rootSigChanged || compDescHandlesBound_[3].ptr != pbs.uavs_.gpuDescHandle_.ptr)
			{
				DBG_ASSERT(pbs.uavs_.gpuDescHandle_.ptr ==
				           (viewHeap->GetGPUDescriptorHandleForHeapStart().ptr + pbs.uavs_.offset_ * viewIncrSize));
				d3dCommandList_->SetComputeRootDescriptorTable(3, pbs.uavs_.gpuDescHandle_);
				compDescHandlesBound_[3] = pbs.uavs_.gpuDescHandle_;
			}
			break;
		default:
			DBG_ASSERT(false);
			return ErrorCode::FAIL;
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetFrameBinding(Handle fbsHandle)
	{
		if(fbsBound_ == fbsHandle)
			return ErrorCode::OK;

		fbsBound_ = fbsHandle;
		const auto& fbs = backend_.frameBindingSets_[fbsHandle.GetIndex()];

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescLocal;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescLocal;
		D3D12_CPU_DESCRIPTOR_HANDLE* rtvDesc = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE* dsvDesc = nullptr;

		const i32 rtvBaseIdx = fbs.swapChain_ == nullptr ? 0 : fbs.swapChain_->bbIdx_ * MAX_BOUND_RTVS;

		if(fbs.numRTs_)
		{
			rtvDesc = &rtvDescLocal;
			rtvDescLocal = fbs.rtvs_.cpuDescHandle_;

			for(i32 i = 0; i < fbs.numRTs_; ++i)
			{
				AddTransition(fbs.rtvResources_[rtvBaseIdx + i], D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			rtvDesc->ptr +=
			    rtvBaseIdx *
			    backend_.device_->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
		if(fbs.desc_.dsv_.resource_)
		{
			dsvDesc = &dsvDescLocal;
			dsvDescLocal = fbs.dsv_.cpuDescHandle_;

			if(Core::ContainsAllFlags(fbs.desc_.dsv_.flags_, DSVFlags::READ_ONLY_DEPTH))
			{
				AddTransition(fbs.dsvResource_, D3D12_RESOURCE_STATE_DEPTH_READ);
			}
			else
			{
				AddTransition(fbs.dsvResource_, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			}
		}

		d3dCommandList_->OMSetRenderTargets(fbs.numRTs_, rtvDesc, TRUE, dsvDesc);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetDrawState(const DrawState* drawState)
	{
		if(cachedDrawState_ == nullptr || drawState != cachedDrawState_)
		{
			if(drawState->viewport_ != cachedViewport_)
			{
				D3D12_VIEWPORT viewport;
				viewport.TopLeftX = drawState->viewport_.x_;
				viewport.TopLeftY = drawState->viewport_.y_;
				viewport.Width = drawState->viewport_.w_;
				viewport.Height = drawState->viewport_.h_;
				viewport.MinDepth = drawState->viewport_.zMin_;
				viewport.MaxDepth = drawState->viewport_.zMax_;
				d3dCommandList_->RSSetViewports(1, &viewport);
				cachedViewport_ = drawState->viewport_;
			}

			if(drawState->scissorRect_ != cachedScissorRect_)
			{
				D3D12_RECT scissorRect;
				scissorRect.left = drawState->scissorRect_.x_;
				scissorRect.top = drawState->scissorRect_.y_;
				scissorRect.right = drawState->scissorRect_.x_ + drawState->scissorRect_.w_;
				scissorRect.bottom = drawState->scissorRect_.y_ + drawState->scissorRect_.h_;
				d3dCommandList_->RSSetScissorRects(1, &scissorRect);
				cachedScissorRect_ = drawState->scissorRect_;
			}

			if(drawState->stencilRef_ != cachedStencilRef_)
			{
				d3dCommandList_->OMSetStencilRef(drawState->stencilRef_);
				cachedStencilRef_ = drawState->stencilRef_;
			}
		}
		return ErrorCode::OK;
	}

	bool D3D12CompileContext::AddTransition(const D3D12SubresourceRange& subRsc, D3D12_RESOURCE_STATES state)
	{
		return AddTransition(subRsc.resource_, subRsc.firstSubRsc_, subRsc.numSubRsc_, state);
	}

	bool D3D12CompileContext::AddTransition(
	    const D3D12Resource* resource, i32 firstSubRsc, i32 numSubRsc, D3D12_RESOURCE_STATES state)
	{
		DBG_ASSERT(resource);
		DBG_ASSERT(resource->resource_);

		bool changed = false;

		for(i32 subRscIdx = firstSubRsc; subRscIdx < (firstSubRsc + numSubRsc); ++subRscIdx)
		{
			Subresource subRsc(resource, subRscIdx);

			auto stateEntry = stateTracker_.find(subRsc);
			if(stateEntry == stateTracker_.end())
			{
				stateEntry = stateTracker_.insert(subRsc, resource->defaultState_);
			}

			auto prevState = stateEntry->second;
			if(state != prevState)
			{
				D3D12_RESOURCE_BARRIER barrier;
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = resource->resource_.Get();
				barrier.Transition.Subresource = subRscIdx;
				barrier.Transition.StateBefore = stateEntry->second;
				barrier.Transition.StateAfter = state;
				pendingBarriers_.insert(Subresource(resource, barrier.Transition.Subresource), barrier);
				stateEntry->second = state;
				changed = true;
			}
		}
		return changed;
	}

	void D3D12CompileContext::AddUAVBarrier(const D3D12SubresourceRange& subRsc)
	{
		DBG_ASSERT(subRsc);

		// Only submit a UAV barrier if there was no change to state.
		if(!AddTransition(subRsc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
		{
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.UAV.pResource = subRsc.resource_->resource_.Get();
			pendingBarriers_.insert(Subresource(subRsc.resource_, -1), barrier);
		}
	}

	void D3D12CompileContext::FlushTransitions()
	{
		if(pendingBarriers_.size() > 0)
		{
#if DEBUG_TRANSITIONS
			Core::Log("FlushTransitions.\n");
#endif
			// Copy pending barriers into flat vector.
			for(auto barrier : pendingBarriers_)
			{
				auto barrierInfo = barrier.second;
				barriers_.push_back(barrierInfo);

#if DEBUG_TRANSITIONS
				switch(barrierInfo.Type)
				{
				case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
					Core::Log("- Transition %p (%u): %x -> %x\n", barrierInfo.Transition.pResource,
					    barrierInfo.Transition.Subresource, barrierInfo.Transition.StateBefore,
					    barrierInfo.Transition.StateAfter);
					break;
				case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
					Core::Log("- Aliasing %p -> %p\n", barrierInfo.Aliasing.pResourceBefore,
					    barrierInfo.Aliasing.pResourceAfter);
					break;
				case D3D12_RESOURCE_BARRIER_TYPE_UAV:
					Core::Log("- UAV %p\n", barrierInfo.UAV.pResource);
					break;
				}

				d3dCommandList_->ResourceBarrier(1, &barrierInfo);
#endif
			}

// Perform resource barriers.
#if !DEBUG_TRANSITIONS
			d3dCommandList_->ResourceBarrier(barriers_.size(), barriers_.data());
#endif
			pendingBarriers_.clear();
			barriers_.clear();
		}
	}

	void D3D12CompileContext::FlushDescriptors()
	{
		if(viewDescCopyParams_.numHandles_.size() > 0)
		{
			auto d3dDevice = backend_.device_->d3dDevice_;
			d3dDevice->CopyDescriptors(viewDescCopyParams_.numHandles_.size(), viewDescCopyParams_.dstHandles_.data(),
			    viewDescCopyParams_.numHandles_.data(), viewDescCopyParams_.numHandles_.size(),
			    viewDescCopyParams_.srcHandles_.data(), viewDescCopyParams_.numHandles_.data(),
			    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			viewDescCopyParams_.dstHandles_.clear();
			viewDescCopyParams_.srcHandles_.clear();
			viewDescCopyParams_.numHandles_.clear();
		}
	}

	void D3D12CompileContext::RestoreDefault()
	{
		for(auto state : stateTracker_)
		{
			auto& subRsc = state.first;
			AddTransition(subRsc.resource_, subRsc.idx_, 1, subRsc.resource_->defaultState_);
		}
		FlushTransitions();
		stateTracker_.clear();
	}

} // namespace GPU
