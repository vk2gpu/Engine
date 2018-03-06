#include "gpu/command_list.h"
#include "gpu/manager.h"
#include "gpu/resources.h"
#include "gpu/utils.h"
#include "client/window.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/vector.h"
#include "plugin/manager.h"

#include "catch.hpp"

namespace
{
	GPU::SetupParams GetDefaultSetupParams()
	{
		GPU::SetupParams setupParams;
		setupParams.debugFlags_ = GPU::DebugFlags::NONE;
		return setupParams;
	}
}

TEST_CASE("transfer-tests-update-copy-readback-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::BufferDesc bufferDesc;
	bufferDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	bufferDesc.size_ = 1024 * 1024;
	GPU::Handle bufferHandle = GPU::Manager::CreateBuffer(bufferDesc, nullptr, testName.c_str());

	GPU::BufferDesc readbackDesc;
	readbackDesc.bindFlags_ = GPU::BindFlags::NONE;
	readbackDesc.size_ = 1024 * 1024;
	GPU::Handle readbackHandle = GPU::Manager::CreateBuffer(readbackDesc, nullptr, testName.c_str());

	GPU::Handle fenceHandle = GPU::Manager::CreateFence(0, testName.c_str());

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	auto commandListHandle = GPU::Manager::CreateCommandList(testName.c_str());

	GPU::CommandList commandList(GPU::CommandList::DEFAULT_BUFFER_SIZE);

	float testData0[] = {1.0f, 2.0f, 3.0f, 4.0f};
	float testData1[] = {0.1f, 0.2f, 0.3f, 0.4f};

	REQUIRE(commandList.UpdateBuffer(bufferHandle, 0, sizeof(testData0), testData0));
	REQUIRE(commandList.UpdateBuffer(bufferHandle, 16, sizeof(testData1), testData1));

	REQUIRE(commandList.CopyBuffer(readbackHandle, 0, bufferHandle, 0, (i32)readbackDesc.size_));

	REQUIRE(GPU::Manager::CompileCommandList(commandListHandle, commandList));

	REQUIRE(GPU::Manager::SubmitCommandList(commandListHandle));

	REQUIRE(GPU::Manager::SubmitFence(fenceHandle, 1));
	REQUIRE(GPU::Manager::WaitOnFence(fenceHandle, 1));

	float readbackData[8] = {};
	REQUIRE(GPU::Manager::ReadbackBuffer(readbackHandle, 0, 32, readbackData));

	REQUIRE(memcmp(testData0, &readbackData[0], sizeof(testData0)) == 0);
	REQUIRE(memcmp(testData1, &readbackData[4], sizeof(testData1)) == 0);

	GPU::Manager::DestroyResource(commandListHandle);
	GPU::Manager::DestroyResource(fenceHandle);
	GPU::Manager::DestroyResource(readbackHandle);
	GPU::Manager::DestroyResource(bufferHandle);
}

TEST_CASE("transfer-tests-update-copy-readback-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::TextureDesc textureDesc;
	textureDesc.type_ = GPU::TextureType::TEX2D;
	textureDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	textureDesc.width_ = 4;
	textureDesc.height_ = 2;
	textureDesc.format_ = GPU::Format::R32_FLOAT;
	GPU::Handle textureHandle = GPU::Manager::CreateTexture(textureDesc, nullptr, testName.c_str());

	GPU::TextureDesc readbackDesc;
	readbackDesc.type_ = GPU::TextureType::TEX2D;
	readbackDesc.bindFlags_ = GPU::BindFlags::NONE;
	readbackDesc.width_ = 4;
	readbackDesc.height_ = 2;
	readbackDesc.format_ = GPU::Format::R32_FLOAT;
	GPU::Handle readbackHandle = GPU::Manager::CreateTexture(readbackDesc, nullptr, testName.c_str());

	GPU::Handle fenceHandle = GPU::Manager::CreateFence(0, testName.c_str());

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	auto commandListHandle = GPU::Manager::CreateCommandList(testName.c_str());

	GPU::CommandList commandList(GPU::CommandList::DEFAULT_BUFFER_SIZE);

	float testData[] = {1.0f, 2.0f, 3.0f, 4.0f, 0.1f, 0.2f, 0.3f, 0.4f};

	GPU::ConstTextureSubResourceData updateData = {};
	updateData.data_ = testData;
	updateData.rowPitch_ = sizeof(float) * 4;
	updateData.slicePitch_ = sizeof(testData);

	REQUIRE(commandList.UpdateTextureSubResource(textureHandle, 0, updateData));

	GPU::Point destPoint = {};
	destPoint.x_ = 0;
	destPoint.y_ = 0;
	destPoint.z_ = 0;

	GPU::Box srcBox = {};
	srcBox.x_ = 0;
	srcBox.y_ = 0;
	srcBox.z_ = 0;
	srcBox.w_ = 4;
	srcBox.h_ = 2;
	srcBox.d_ = 1;

	REQUIRE(commandList.CopyTextureSubResource(readbackHandle, 0, destPoint, textureHandle, 0, srcBox));

	REQUIRE(GPU::Manager::CompileCommandList(commandListHandle, commandList));

	REQUIRE(GPU::Manager::SubmitCommandList(commandListHandle));

	REQUIRE(GPU::Manager::SubmitFence(fenceHandle, 1));
	REQUIRE(GPU::Manager::WaitOnFence(fenceHandle, 1));

	float readbackDataBuffer[8] = {};
	GPU::TextureSubResourceData readbackData = {};
	readbackData.data_ = readbackDataBuffer;
	readbackData.rowPitch_ = sizeof(float) * 4;
	readbackData.slicePitch_ = sizeof(readbackDataBuffer);

	REQUIRE(GPU::Manager::ReadbackTextureSubresource(readbackHandle, 0, readbackData));

	REQUIRE(memcmp(testData, &readbackDataBuffer[0], sizeof(testData)) == 0);

	GPU::Manager::DestroyResource(commandListHandle);
	GPU::Manager::DestroyResource(fenceHandle);
	GPU::Manager::DestroyResource(readbackHandle);
	GPU::Manager::DestroyResource(textureHandle);
}
