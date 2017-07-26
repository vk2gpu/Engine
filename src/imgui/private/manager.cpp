#include "imgui/manager.h"
#include "client/input_provider.h"
#include "client/key_input.h"
#include "math/mat44.h"

#include "gpu/manager.h"

typedef u8 BYTE;
#include "imgui/private/shaders/imgui_vs.h"
#include "imgui/private/shaders/imgui_ps.h"

namespace ImGui
{
	namespace
	{
		static const i32 MAX_VERTICES = 1024 * 1024;
		static const i32 MAX_INDICES = 1024 * 1024;

		int keyMap_[ImGuiKey_COUNT];

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

		bool isInitialized_ = false;
	}

	void Manager::Initialize()
	{
		DBG_ASSERT(GPU::Manager::IsInitialized());
		DBG_ASSERT(GPU::Manager::IsAdapterCreated());

		ImGuiIO& IO = ImGui::GetIO();

		GPU::BufferDesc vbDesc;
		vbDesc.size_ = MAX_VERTICES * sizeof(ImDrawVert);
		vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		vbHandle_ = GPU::Manager::CreateBuffer(vbDesc, nullptr, "ImGui VB");

		GPU::BufferDesc ibDesc;
		ibDesc.size_ = MAX_INDICES * sizeof(ImDrawIdx);
		ibDesc.bindFlags_ = GPU::BindFlags::INDEX_BUFFER;
		ibHandle_ = GPU::Manager::CreateBuffer(ibDesc, nullptr, "ImGui IB");

		GPU::DrawBindingSetDesc dbsDesc;
		dbsDesc.vbs_[0].offset_ = 0;
		dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;
		dbsDesc.vbs_[0].stride_ = sizeof(ImDrawVert);
		dbsDesc.vbs_[0].resource_ = vbHandle_;
		dbsDesc.ib_.offset_ = 0;
		dbsDesc.ib_.size_ = (i32)ibDesc.size_;
		dbsDesc.ib_.stride_ = sizeof(ImDrawIdx);
		dbsDesc.ib_.resource_ = ibHandle_;
		dbsHandle_ = GPU::Manager::CreateDrawBindingSet(dbsDesc, "ImGui DBS");

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
		fontHandle_ = GPU::Manager::CreateTexture(fontDesc, &texSubRscData, "ImGui Font Texture");
		DBG_ASSERT(fontHandle_);

		GPU::ShaderDesc vsDesc;
		vsDesc.type_ = GPU::ShaderType::VS;
		vsDesc.data_ = g_VShader;
		vsDesc.dataSize_ = sizeof(g_VShader);
		vsHandle_ = GPU::Manager::CreateShader(vsDesc, "ImGui VS");
		DBG_ASSERT(vsHandle_);

		GPU::ShaderDesc psDesc;
		psDesc.type_ = GPU::ShaderType::PS;
		psDesc.data_ = g_PShader;
		psDesc.dataSize_ = sizeof(g_PShader);
		psHandle_ = GPU::Manager::CreateShader(psDesc, "ImGui PS");
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
		gpsHandle_ = GPU::Manager::CreateGraphicsPipelineState(gpsDesc, "ImGui GPS");
		DBG_ASSERT(gpsHandle_);

		GPU::SamplerState smpDesc;
		smpHandle_ = GPU::Manager::CreateSamplerState(smpDesc, "ImGui Sampler");
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
		pbsHandle_ = GPU::Manager::CreatePipelineBindingSet(pbsDesc, "ImGui PBS");

		// Setup keymap.
		keyMap_[ImGuiKey_Tab] = (int)Client::KeyCode::TAB;
		keyMap_[ImGuiKey_LeftArrow] = (i32)Client::KeyCode::LEFT;
		keyMap_[ImGuiKey_RightArrow] = (i32)Client::KeyCode::RIGHT;
		;
		keyMap_[ImGuiKey_UpArrow] = (i32)Client::KeyCode::UP;
		keyMap_[ImGuiKey_DownArrow] = (i32)Client::KeyCode::DOWN;
		keyMap_[ImGuiKey_PageUp] = (i32)Client::KeyCode::PAGEUP;
		keyMap_[ImGuiKey_PageDown] = (i32)Client::KeyCode::PAGEDOWN;
		keyMap_[ImGuiKey_Home] = (i32)Client::KeyCode::HOME;
		keyMap_[ImGuiKey_End] = (i32)Client::KeyCode::END;
		keyMap_[ImGuiKey_Delete] = (i32)Client::KeyCode::DELETE;
		keyMap_[ImGuiKey_Backspace] = (i32)Client::KeyCode::BACKSPACE;
		keyMap_[ImGuiKey_Enter] = (i32)Client::KeyCode::RETURN;
		keyMap_[ImGuiKey_Escape] = (i32)Client::KeyCode::ESCAPE;
		keyMap_[ImGuiKey_A] = (i32)Client::KeyCode::CHAR_a;
		keyMap_[ImGuiKey_C] = (i32)Client::KeyCode::CHAR_c;
		keyMap_[ImGuiKey_V] = (i32)Client::KeyCode::CHAR_v;
		keyMap_[ImGuiKey_X] = (i32)Client::KeyCode::CHAR_x;
		keyMap_[ImGuiKey_Y] = (i32)Client::KeyCode::CHAR_y;
		keyMap_[ImGuiKey_Z] = (i32)Client::KeyCode::CHAR_z;
		for(i32 i = 0; i < ImGuiKey_COUNT; ++i)
			IO.KeyMap[i] = i;


#if PLATFORM_OSX
		IO.OSXBehaviors = true;
#endif

		// Setup custom style.
		auto& style = ImGui::GetStyle();
		style.WindowPadding = Math::Vec2(2.0f, 2.0f);
		style.WindowRounding = 2.0f;
		style.FramePadding = Math::Vec2(2.0f, 2.0f);
		style.ItemSpacing = Math::Vec2(8.0f, 4.0f);
		style.IndentSpacing = 16.0f;
		style.ScrollbarSize = 16.0f;
		style.ScrollbarRounding = 4.0f;

		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.71f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.08f, 0.07f, 0.07f, 0.87f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.46f, 0.52f, 0.52f, 0.61f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.90f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 0.45f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.29f, 0.29f, 0.29f, 0.83f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.31f, 0.45f, 0.87f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.80f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.52f, 0.52f, 0.67f, 0.60f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.60f, 0.76f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.72f, 0.72f, 0.80f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.90f, 0.90f, 0.90f, 0.45f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.72f, 0.90f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.69f, 0.69f, 0.87f, 0.80f);
		style.Colors[ImGuiCol_Column] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
		style.Colors[ImGuiCol_CloseButton] = ImVec4(0.50f, 0.50f, 0.90f, 0.50f);
		style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
		style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.35f, 0.35f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.37f, 0.37f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.38f, 0.38f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.45f, 0.45f, 1.00f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		isInitialized_ = true;
	}

	void Manager::Finalize()
	{
		DBG_ASSERT(IsInitialized());

		GPU::Manager::DestroyResource(pbsHandle_);
		GPU::Manager::DestroyResource(smpHandle_);
		GPU::Manager::DestroyResource(gpsHandle_);
		GPU::Manager::DestroyResource(psHandle_);
		GPU::Manager::DestroyResource(vsHandle_);
		GPU::Manager::DestroyResource(fontHandle_);
		GPU::Manager::DestroyResource(dbsHandle_);
		GPU::Manager::DestroyResource(ibHandle_);
		GPU::Manager::DestroyResource(vbHandle_);

		isInitialized_ = false;
	}

	bool Manager::IsInitialized() { return isInitialized_; }

	void Manager::BeginFrame(const Client::IInputProvider& input, i32 w, i32 h)
	{
		DBG_ASSERT(IsInitialized());

		ImGuiIO& IO = ImGui::GetIO();
		IO.DisplaySize.x = (f32)w;
		IO.DisplaySize.y = (f32)h;

		// Setup mouse position.
		IO.MousePos = input.GetMousePosition();
		IO.MouseDown[0] = input.IsMouseButtonDown(0);
		IO.MouseDown[1] = input.IsMouseButtonDown(1);
		IO.MouseDown[2] = input.IsMouseButtonDown(2);
		IO.MouseDown[3] = input.IsMouseButtonDown(3);
		IO.MouseDown[4] = input.IsMouseButtonDown(4);
		IO.MouseWheel = input.GetMouseWheelDelta().y;

		// Handle keyboard input.
		IO.KeyCtrl = input.IsKeyDown(Client::KeyCode::LCTRL) || input.IsKeyDown(Client::KeyCode::RCTRL);
		IO.KeyShift = input.IsKeyDown(Client::KeyCode::LSHIFT) || input.IsKeyDown(Client::KeyCode::RSHIFT);
		IO.KeyAlt = input.IsKeyDown(Client::KeyCode::LALT) || input.IsKeyDown(Client::KeyCode::RALT);
		IO.KeySuper = input.IsKeyDown(Client::KeyCode::LGUI) || input.IsKeyDown(Client::KeyCode::RGUI);

		for(i32 i = 0; i < ImGuiKey_COUNT; ++i)
		{
			if(IO.KeyMap[i] != -1)
				IO.KeysDown[i] = input.IsKeyDown(keyMap_[i]);
			if(IO.KeysDown[i])
				i = i;
		}

		char textBuffer[4096] = {0};
		i32 numBytes = input.GetTextInput(textBuffer, sizeof(textBuffer));
		if(numBytes > 0)
		{
			IO.AddInputCharactersUTF8(textBuffer);
		}

		ImGui::NewFrame();
	}

	void Manager::EndFrame(const GPU::Handle& fbs, GPU::CommandList& cmdList)
	{
		DBG_ASSERT(IsInitialized());

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

} // namespace ImGui
