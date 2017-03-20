#include "imgui/imgui.h"
#include "math/mat44.h"

#include "gpu/manager.h"

typedef u8 BYTE;
#include "imgui/private/shaders/imgui_vs.h"
#include "imgui/private/shaders/imgui_ps.h"

namespace ImGui
{
	namespace
	{
		GPU::Manager* gpuManager_ = nullptr;

		static const i32 MAX_VERTICES = 1024 * 64;
		static const i32 MAX_INDICES = 1024 * 64;

		// GPU resources.
		GPU::Handle vbHandle_;
		GPU::Handle ibHandle_;
		GPU::Handle dbsHandle_;
		GPU::Handle fontHandle_;
		GPU::Handle vsHandle_;
		GPU::Handle psHandle_;
		GPU::Handle gpsHandle_;
		GPU::Handle smpHandle_;
		GPU::Handle pbsHandle_;
	}

	void Initialize(GPU::Manager& gpuManager)
	{
		gpuManager_ = &gpuManager;

		ImGuiIO& IO = ImGui::GetIO();

		GPU::BufferDesc vbDesc;
		vbDesc.size_ = MAX_VERTICES * sizeof(ImDrawVert);
		vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		vbHandle_ = gpuManager.CreateBuffer(vbDesc, nullptr, "ImGui VB");

		GPU::BufferDesc ibDesc;
		ibDesc.size_ = MAX_INDICES * sizeof(ImDrawIdx);
		ibDesc.bindFlags_ = GPU::BindFlags::INDEX_BUFFER;
		ibHandle_ = gpuManager.CreateBuffer(ibDesc, nullptr, "ImGui IB");

		GPU::DrawBindingSetDesc dbsDesc;
		dbsDesc.vbs_[0].offset_ = 0;
		dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;
		dbsDesc.vbs_[0].stride_ = sizeof(ImDrawVert);
		dbsDesc.vbs_[0].resource_ = vbHandle_;
		dbsDesc.ib_.offset_ = 0;
		dbsDesc.ib_.size_ = (i32)ibDesc.size_;
		dbsDesc.ib_.stride_ = sizeof(ImDrawIdx);
		dbsDesc.ib_.resource_ = ibHandle_;
		dbsHandle_ = gpuManager.CreateDrawBindingSet(dbsDesc, "ImGui DBS");

		GPU::TextureDesc fontDesc;
		unsigned char* pixels = nullptr;
		int width, height;
		IO.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		fontDesc.type_ = GPU::TextureType::TEX2D;
		fontDesc.width_ = width;
		fontDesc.height_ = height;
		fontDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
		GPU::TextureSubResourceData texSubRscData;
		texSubRscData.data_ = pixels;
		texSubRscData.rowPitch_ = width * sizeof(u32);
		texSubRscData.slicePitch_ = 0;
		fontHandle_ = gpuManager.CreateTexture(fontDesc, &texSubRscData, "ImGui Font Texture");
		DBG_ASSERT(fontHandle_);

		GPU::ShaderDesc vsDesc;
		vsDesc.type_ = GPU::ShaderType::VERTEX;
		vsDesc.data_ = g_VShader;
		vsDesc.dataSize_ = sizeof(g_VShader);
		vsHandle_ = gpuManager.CreateShader(vsDesc, "ImGui VS");
		DBG_ASSERT(vsHandle_);

		GPU::ShaderDesc psDesc;
		psDesc.type_ = GPU::ShaderType::PIXEL;
		psDesc.data_ = g_PShader;
		psDesc.dataSize_ = sizeof(g_PShader);
		psHandle_ = gpuManager.CreateShader(psDesc, "ImGui PS");
		DBG_ASSERT(psHandle_);

		GPU::GraphicsPipelineStateDesc gpsDesc;
		gpsDesc.shaders_[(i32)vsDesc.type_] = vsHandle_;
		gpsDesc.shaders_[(i32)psDesc.type_] = psHandle_;
		gpsDesc.renderState_.blendStates_[0].enable_ = true;
		gpsDesc.renderState_.blendStates_[0].srcBlend_ = GPU::BlendType::SRC_ALPHA;
		gpsDesc.renderState_.blendStates_[0].srcBlendAlpha_ = GPU::BlendType::SRC_ALPHA;
		gpsDesc.renderState_.blendStates_[0].destBlend_ = GPU::BlendType::INV_SRC_ALPHA;
		gpsDesc.renderState_.blendStates_[0].destBlendAlpha_ = GPU::BlendType::INV_SRC_ALPHA;
		gpsDesc.numVertexElements_ = 3;
		gpsDesc.vertexElements_[0].usage_ = GPU::VertexUsage::POSITION;
		gpsDesc.vertexElements_[0].usageIdx_ = 0;
		gpsDesc.vertexElements_[0].streamIdx_ = 0;
		gpsDesc.vertexElements_[0].format_ = GPU::Format::R32G32_FLOAT;
		gpsDesc.vertexElements_[0].offset_ = 0;
		gpsDesc.vertexElements_[1].usage_ = GPU::VertexUsage::TEXCOORD;
		gpsDesc.vertexElements_[1].usageIdx_ = 0;
		gpsDesc.vertexElements_[1].streamIdx_ = 0;
		gpsDesc.vertexElements_[1].format_ = GPU::Format::R32G32_FLOAT;
		gpsDesc.vertexElements_[1].offset_ = 8;
		gpsDesc.vertexElements_[2].usage_ = GPU::VertexUsage::COLOR;
		gpsDesc.vertexElements_[2].usageIdx_ = 0;
		gpsDesc.vertexElements_[2].streamIdx_ = 0;
		gpsDesc.vertexElements_[2].format_ = GPU::Format::R8G8B8A8_UNORM;
		gpsDesc.vertexElements_[2].offset_ = 16;
		gpsDesc.topology_ = GPU::TopologyType::TRIANGLE;
		gpsDesc.numRTs_ = 1;
		gpsDesc.rtvFormats_[0] = GPU::Format::R8G8B8A8_UNORM;
		gpsHandle_ = gpuManager.CreateGraphicsPipelineState(gpsDesc, "ImGui GPS");
		DBG_ASSERT(gpsHandle_);

		GPU::SamplerState smpDesc;
		smpHandle_ = gpuManager.CreateSamplerState(smpDesc, "ImGui Sampler");
		DBG_ASSERT(smpHandle_);

		GPU::PipelineBindingSetDesc pbsDesc;
		pbsDesc.pipelineState_ = gpsHandle_;
		pbsDesc.numSRVs_ = 1;
		pbsDesc.numSamplers_ = 1;
		pbsDesc.srvs_[0].resource_ = fontHandle_;
		pbsDesc.srvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
		pbsDesc.srvs_[0].format_ = fontDesc.format_;
		pbsDesc.srvs_[0].mipLevels_NumElements_ = -1;
		pbsDesc.samplers_[0].resource_ = smpHandle_;
		pbsHandle_ = gpuManager.CreatePipelineBindingSet(pbsDesc, "ImGui PBS");
	}

	void BeginFrame(i32 w, i32 h)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DisplaySize.x = (f32)w;
		IO.DisplaySize.y = (f32)h;
		ImGui::NewFrame();
	}

	void EndFrame(const GPU::Handle& fbs, GPU::CommandList& cmdList)
	{
		ImGui::Render();

		ImGuiIO& IO = ImGui::GetIO();
		ImDrawData* drawData = ImGui::GetDrawData();
		if(drawData == nullptr)
			return;

		// Calculate size of updates.
		i32 vbUpdateSize = 0;
		i32 ibUpdateSize = 0;
		for(int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
		{
			const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
			vbUpdateSize += drawList->VtxBuffer.size();
			ibUpdateSize += drawList->IdxBuffer.size();
		}

		Math::Mat44 clipTranform;
		clipTranform.OrthoProjection(0.0f, IO.DisplaySize.x, IO.DisplaySize.y, 0.0f, -1.0f, 1.0f);

		GPU::DrawState drawState;
		drawState.viewport_.w_ = IO.DisplaySize.x;
		drawState.viewport_.h_ = IO.DisplaySize.y;
		drawState.scissorRect_.w_ = (i32)IO.DisplaySize.x;
		drawState.scissorRect_.h_ = (i32)IO.DisplaySize.y;

		// Allocate some vertices + indices from the command list.
		auto* baseVertices = (ImDrawVert*)cmdList.Alloc(vbUpdateSize * sizeof(ImDrawVert));
		auto* baseIndices = (ImDrawIdx*)cmdList.Alloc(ibUpdateSize * sizeof(ImDrawIdx));
		auto* vertices = baseVertices;
		auto* indices = baseIndices;

		i32 noofVertices = 0;
		for(i32 cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
		{
			const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
			memcpy(vertices, &drawList->VtxBuffer[0], drawList->VtxBuffer.size() * sizeof(ImDrawVert));

			// No uniform buffer slot in shader, so we need to transform before the vertex shader.
			for(int VertIdx = 0; VertIdx < drawList->VtxBuffer.size(); ++VertIdx)
			{
				vertices[VertIdx].pos = vertices[VertIdx].pos * clipTranform;
			}

			vertices += drawList->VtxBuffer.size();
			noofVertices += drawList->VtxBuffer.size();
			DBG_ASSERT(noofVertices < MAX_VERTICES);
		}

		cmdList.UpdateBuffer(vbHandle_, 0, noofVertices * sizeof(ImDrawVert), baseVertices);

		static_assert(sizeof(ImDrawIdx) == sizeof(u16), "Indices must be 16 bit.");
		i32 noofIndices = 0;
		i32 vertexOffset = 0;
		for(int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
		{
			const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
			for(int Idx = 0; Idx < drawList->IdxBuffer.size(); ++Idx)
			{
				i32 index = vertexOffset + static_cast<i32>(drawList->IdxBuffer[Idx]);
				DBG_ASSERT(index < 0xffff);
				indices[Idx] = static_cast<ImDrawIdx>(index);
			}
			indices += drawList->IdxBuffer.size();
			noofIndices += drawList->IdxBuffer.size();
			vertexOffset += drawList->VtxBuffer.size();
			DBG_ASSERT(noofIndices < MAX_INDICES);
		}

		cmdList.UpdateBuffer(ibHandle_, 0, noofIndices * sizeof(ImDrawVert), baseIndices);

		u32 indexOffset = 0;
		for(int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
		{
			const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];

			for(int cmdIdx = 0; cmdIdx < drawList->CmdBuffer.size(); ++cmdIdx)
			{
				const ImDrawCmd* cmd = &drawList->CmdBuffer[cmdIdx];
				if(cmd->UserCallback)
				{
					cmd->UserCallback(drawList, cmd);
				}
				else
				{
					GPU::ScissorRect scissorRect;
					drawState.scissorRect_.x_ = (i32)cmd->ClipRect.x;
					drawState.scissorRect_.y_ = (i32)cmd->ClipRect.y;
					drawState.scissorRect_.w_ = (i32)(cmd->ClipRect.z - cmd->ClipRect.x);
					drawState.scissorRect_.h_ = (i32)(cmd->ClipRect.w - cmd->ClipRect.y);

					cmdList.Draw(pbsHandle_, dbsHandle_, fbs, drawState, GPU::PrimitiveTopology::TRIANGLE_LIST,
					    indexOffset, 0, cmd->ElemCount, 0, 1);
				}
				indexOffset += cmd->ElemCount;
			}
		}
	}

	void Finalize()
	{
		gpuManager_->DestroyResource(pbsHandle_);
		gpuManager_->DestroyResource(smpHandle_);
		gpuManager_->DestroyResource(gpsHandle_);
		gpuManager_->DestroyResource(psHandle_);
		gpuManager_->DestroyResource(vsHandle_);
		gpuManager_->DestroyResource(fontHandle_);
		gpuManager_->DestroyResource(dbsHandle_);
		gpuManager_->DestroyResource(ibHandle_);
		gpuManager_->DestroyResource(vbHandle_);

		gpuManager_ = nullptr;
	}

} // namespace ImGui
