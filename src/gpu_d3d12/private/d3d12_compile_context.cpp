#include "gpu_d3d12/d3d12_compile_context.h"
#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_backend.h"
#include "gpu_d3d12/d3d12_device.h"
#include "gpu_d3d12/d3d12_linear_heap_allocator.h"
#include "gpu/command_list.h"
#include "gpu/commands.h"

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
				default:
					DBG_BREAK;
				}
			}

#undef CASE_COMMAND

			RestoreDefault();
			return outCommandList.Close();
		}
		return ErrorCode::FAIL;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDraw* command)
	{
		const auto& dbs = backend_.drawBindingSets_[command->drawBinding_.GetIndex()];

		SetPipelineBinding(command->pipelineBinding_);
		SetFrameBinding(command->frameBinding_);
		SetDrawBinding(command->drawBinding_, command->primitive_);

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
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDrawIndirect* command)
	{
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDispatch* command)
	{
		SetPipelineBinding(command->pipelineBinding_);
		d3dCommandList_->Dispatch(command->xGroups_, command->yGroups_, command->zGroups_);
		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandDispatchIndirect* command)
	{
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearRTV* command)
	{
		const auto& fbs = backend_.frameBindingSets_[command->frameBinding_.GetIndex()];
		DBG_ASSERT(command->rtvIdx_ < fbs.numRTs_);


		D3D12_CPU_DESCRIPTOR_HANDLE handle = fbs.rtvs_.cpuDescHandle_;
		handle.ptr += command->rtvIdx_ *
		              backend_.device_->d3dDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		AddTransition(&fbs.rtvResources_[command->rtvIdx_], D3D12_RESOURCE_STATE_RENDER_TARGET);
		FlushTransitions();

		d3dCommandList_->ClearRenderTargetView(handle, command->color_, 0, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearDSV* command)
	{
		const auto& fbs = backend_.frameBindingSets_[command->frameBinding_.GetIndex()];
		DBG_ASSERT(fbs.desc_.dsv_.resource_);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = fbs.dsv_.cpuDescHandle_;

		AddTransition(&fbs.dsvResource_, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		FlushTransitions();

		d3dCommandList_->ClearDepthStencilView(
		    handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, command->depth_, command->stencil_, 0, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandClearUAV* command) { return ErrorCode::UNIMPLEMENTED; }

	ErrorCode D3D12CompileContext::CompileCommand(const CommandUpdateBuffer* command)
	{
		const auto& buf = backend_.bufferResources_[command->buffer_.GetIndex()];
		DBG_ASSERT(buf.resource_);

		auto uploadAlloc = backend_.device_->GetUploadAllocator().Alloc(command->size_);
		memcpy(uploadAlloc.address_, command->data_, command->size_);

		AddTransition(&buf, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushTransitions();

		d3dCommandList_->CopyBufferRegion(buf.resource_.Get(), command->offset_, uploadAlloc.baseResource_.Get(),
		    uploadAlloc.offsetInBaseResource_, command->size_);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandUpdateTextureSubResource* command)
	{
		const auto& tex = backend_.textureResources_[command->texture_.GetIndex()];
		DBG_ASSERT(tex.resource_);


		auto srcLayout = command->data_;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT dstLayout;
		i32 numRows = 0;
		i64 rowSizeInBytes = 0;
		i64 totalBytes = 0;
		D3D12_RESOURCE_DESC resDesc = GetResourceDesc(tex.desc_);

		backend_.device_->d3dDevice_->GetCopyableFootprints(&resDesc, command->subResourceIdx_, 1, 0, &dstLayout,
		    (u32*)&numRows, (u64*)&rowSizeInBytes, (u64*)&totalBytes);

		auto resAlloc = backend_.device_->GetUploadAllocator().Alloc(totalBytes);
		DBG_ASSERT(srcLayout.rowPitch_ <= rowSizeInBytes);
		const u8* srcData = (const u8*)command->data_.data_;
		u8* dstData = (u8*)resAlloc.address_ + dstLayout.Offset;
		for(i32 slice = 0; slice < tex.desc_.depth_; ++slice)
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
		dst.pResource = tex.resource_.Get();
		dst.SubresourceIndex = command->subResourceIdx_;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = resAlloc.baseResource_.Get();
		src.PlacedFootprint = dstLayout;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		AddTransition(&tex, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushTransitions();
		d3dCommandList_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::CompileCommand(const CommandCopyBuffer* command) { return ErrorCode::UNIMPLEMENTED; }

	ErrorCode D3D12CompileContext::CompileCommand(const CommandCopyTextureSubResource* command)
	{
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12CompileContext::SetDrawBinding(Handle dbsHandle, PrimitiveTopology primitive)
	{
		const auto& dbs = backend_.drawBindingSets_[dbsHandle.GetIndex()];

		// Setup draw binding.
		if(dbs.ib_.BufferLocation)
		{
			AddTransition(&dbs.ibResource_, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			d3dCommandList_->IASetIndexBuffer(&dbs.ib_);
		}

		for(i32 i = 0; i < MAX_VERTEX_STREAMS; ++i)
			if(dbs.vbResources_[i].resource_)
				AddTransition(&dbs.vbResources_[i], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		d3dCommandList_->IASetVertexBuffers(0, MAX_VERTEX_STREAMS, dbs.vbs_.data());
		d3dCommandList_->IASetPrimitiveTopology(GetPrimitiveTopology(primitive));


		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetPipelineBinding(Handle pbsHandle)
	{
		const auto& pbs = backend_.pipelineBindingSets_[pbsHandle.GetIndex()];

		ID3D12DescriptorHeap* heaps[] = {
		    pbs.samplers_.d3dDescriptorHeap_.Get(), pbs.srvs_.d3dDescriptorHeap_.Get(),
		};

		d3dCommandList_->SetDescriptorHeaps(2, heaps);
		d3dCommandList_->SetPipelineState(pbs.pipelineState_.Get());

		switch(pbs.rootSignature_)
		{
		case RootSignatureType::GRAPHICS:
			d3dCommandList_->SetGraphicsRootSignature(
			    backend_.device_->d3dRootSignatures_[(i32)pbs.rootSignature_].Get());
			d3dCommandList_->SetGraphicsRootDescriptorTable(0, pbs.samplers_.gpuDescHandle_);
			d3dCommandList_->SetGraphicsRootDescriptorTable(1, pbs.cbvs_.gpuDescHandle_);
			d3dCommandList_->SetGraphicsRootDescriptorTable(2, pbs.srvs_.gpuDescHandle_);
			d3dCommandList_->SetGraphicsRootDescriptorTable(3, pbs.uavs_.gpuDescHandle_);
			break;
		case RootSignatureType::COMPUTE:
			d3dCommandList_->SetComputeRootSignature(
			    backend_.device_->d3dRootSignatures_[(i32)pbs.rootSignature_].Get());
			d3dCommandList_->SetComputeRootDescriptorTable(0, pbs.samplers_.gpuDescHandle_);
			d3dCommandList_->SetComputeRootDescriptorTable(1, pbs.cbvs_.gpuDescHandle_);
			d3dCommandList_->SetComputeRootDescriptorTable(2, pbs.srvs_.gpuDescHandle_);
			d3dCommandList_->SetComputeRootDescriptorTable(3, pbs.uavs_.gpuDescHandle_);
			break;
		default:
			DBG_BREAK;
			return ErrorCode::FAIL;
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12CompileContext::SetFrameBinding(Handle fbsHandle)
	{
		const auto& fbs = backend_.frameBindingSets_[fbsHandle.GetIndex()];

		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvDesc = nullptr;
		const D3D12_CPU_DESCRIPTOR_HANDLE* dsvDesc = nullptr;
		if(fbs.numRTs_)
		{
			rtvDesc = &fbs.rtvs_.cpuDescHandle_;

			for(i32 i = 0; i < fbs.numRTs_; ++i)
			{
				AddTransition(&fbs.rtvResources_[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
		}
		if(fbs.desc_.dsv_.resource_)
		{
			dsvDesc = &fbs.dsv_.cpuDescHandle_;

			if(Core::ContainsAllFlags(fbs.desc_.dsv_.flags_, DSVFlags::READ_ONLY_DEPTH))
			{
				AddTransition(&fbs.dsvResource_, D3D12_RESOURCE_STATE_DEPTH_READ);
			}
			else
			{
				AddTransition(&fbs.dsvResource_, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			}
		}

		FlushTransitions();

		d3dCommandList_->OMSetRenderTargets(fbs.numRTs_, rtvDesc, TRUE, dsvDesc);

		D3D12_VIEWPORT viewport;
		viewport.TopLeftX = fbs.viewport_.x_;
		viewport.TopLeftY = fbs.viewport_.y_;
		viewport.Width = fbs.viewport_.w_;
		viewport.Height = fbs.viewport_.h_;
		viewport.MinDepth = fbs.viewport_.zMin_;
		viewport.MaxDepth = fbs.viewport_.zMax_;
		d3dCommandList_->RSSetViewports(1, &viewport);

		D3D12_RECT scissorRect;
		scissorRect.left = fbs.scissorRect_.x_;
		scissorRect.top = fbs.scissorRect_.y_;
		scissorRect.right = fbs.scissorRect_.x_ + fbs.scissorRect_.w_;
		scissorRect.bottom = fbs.scissorRect_.y_ + fbs.scissorRect_.h_;
		d3dCommandList_->RSSetScissorRects(1, &scissorRect);

		return ErrorCode::OK;
	}

	void D3D12CompileContext::AddTransition(const D3D12Resource* resource, D3D12_RESOURCE_STATES state)
	{
		DBG_ASSERT(resource);
		DBG_ASSERT(resource->resource_);

		auto stateEntry = stateTracker_.find(resource);
		if(stateEntry == stateTracker_.end())
		{
			stateEntry = stateTracker_.insert(resource, resource->defaultState_);
		}

		if(state != stateEntry->Second_)
		{
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource->resource_.Get();
			barrier.Transition.Subresource = 0xffffffff; // TODO.
			barrier.Transition.StateBefore = stateEntry->Second_;
			barrier.Transition.StateAfter = state;
			pendingBarriers_.insert(resource, barrier);
			stateEntry->Second_ = state;
		}
	}

	void D3D12CompileContext::FlushTransitions()
	{
		if(pendingBarriers_.size() > 0)
		{
			// Copy pending barriers into flat vector.
			for(auto barrier : pendingBarriers_)
			{
				barriers_.push_back(barrier.Second_);
			}
			pendingBarriers_.clear();

			// Perform resource barriers.
			d3dCommandList_->ResourceBarrier(barriers_.size(), barriers_.data());
			barriers_.clear();
		}
	}

	void D3D12CompileContext::RestoreDefault()
	{
		for(auto state : stateTracker_)
		{
			AddTransition(state.First_, state.First_->defaultState_);
		}
		FlushTransitions();
		stateTracker_.clear();
	}

} // namespace GPU
