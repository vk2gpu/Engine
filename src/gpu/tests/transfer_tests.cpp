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

TEST_CASE("transfer-tests-readback-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	Core::Vector<u8> initialData(1024 * 1024);
	for(i32 idx = 0; idx < initialData.size(); ++idx)
	{
		initialData[idx] = (u8)(idx % 255);
	}

	GPU::BufferDesc bufferDesc;
	bufferDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	bufferDesc.size_ = initialData.size();
	GPU::Handle bufferHandle = GPU::Manager::CreateBuffer(bufferDesc, initialData.data(), testName.c_str());

	GPU::BufferDesc readbackDesc = bufferDesc;
	readbackDesc.bindFlags_ = GPU::BindFlags::NONE;
	GPU::Handle readbackHandle = GPU::Manager::CreateBuffer(readbackDesc, nullptr, testName.c_str());

	GPU::Handle fenceHandle = GPU::Manager::CreateFence(0, testName.c_str());

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	auto commandListHandle = GPU::Manager::CreateCommandList(testName.c_str());

	SECTION("copy all - readback all")
	{
		GPU::CommandList commandList(GPU::CommandList::DEFAULT_BUFFER_SIZE);

		REQUIRE(commandList.CopyBuffer(readbackHandle, 0, bufferHandle, 0, (i32)readbackDesc.size_));

		REQUIRE(GPU::Manager::CompileCommandList(commandListHandle, commandList));

		REQUIRE(GPU::Manager::SubmitCommandList(commandListHandle));

		REQUIRE(GPU::Manager::SubmitFence(fenceHandle, 1));
		REQUIRE(GPU::Manager::WaitOnFence(fenceHandle, 1));

		Core::Vector<u8> readbackData(initialData.size());

		REQUIRE(GPU::Manager::ReadbackBuffer(readbackHandle, 0, readbackData.size(), readbackData.data()));

		REQUIRE(memcmp(initialData.data(), readbackData.data(), readbackData.size()) == 0);
	}

	SECTION("copy all - readback half")
	{
		GPU::CommandList commandList(GPU::CommandList::DEFAULT_BUFFER_SIZE);

		REQUIRE(commandList.CopyBuffer(readbackHandle, 0, bufferHandle, 0, (i32)readbackDesc.size_));

		REQUIRE(GPU::Manager::CompileCommandList(commandListHandle, commandList));

		REQUIRE(GPU::Manager::SubmitCommandList(commandListHandle));

		REQUIRE(GPU::Manager::SubmitFence(fenceHandle, 1));
		REQUIRE(GPU::Manager::WaitOnFence(fenceHandle, 1));

		Core::Vector<u8> readbackData(initialData.size());

		REQUIRE(GPU::Manager::ReadbackBuffer(readbackHandle, 0, readbackData.size() / 2, readbackData.data()));
		REQUIRE(memcmp(initialData.data(), readbackData.data(), readbackData.size() / 2) == 0);

		REQUIRE(GPU::Manager::ReadbackBuffer(
		    readbackHandle, readbackData.size() / 2, readbackData.size() / 2, readbackData.data()));
		REQUIRE(
		    memcmp(initialData.data() + readbackData.size() / 2, readbackData.data(), readbackData.size() / 2) == 0);
	}


	GPU::Manager::DestroyResource(commandListHandle);
	GPU::Manager::DestroyResource(fenceHandle);
	GPU::Manager::DestroyResource(readbackHandle);
	GPU::Manager::DestroyResource(bufferHandle);
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

TEST_CASE("transfer-tests-readback-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	static const i32 width = 128;
	static const i32 height = 128;
	static const i32 levels = 2;
	static const i64 totalBytes = GPU::GetTextureSize(GPU::Format::R8_UINT, width, height, 1, levels, 1);

	Core::Vector<u8> initialData((i32)totalBytes);
	for(i32 idx = 0; idx < initialData.size(); ++idx)
	{
		initialData[idx] = (u8)(idx % 255);
	}

	GPU::ConstTextureSubResourceData initialDataSubRsc[] = {{initialData.data(), width, width * height},
	    {initialData.data() + (width * height), width / 2, (width / 2) * (height / 2)}};

	GPU::TextureDesc textureDesc;
	textureDesc.type_ = GPU::TextureType::TEX2D;
	textureDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	textureDesc.width_ = width;
	textureDesc.height_ = height;
	textureDesc.levels_ = levels;
	textureDesc.format_ = GPU::Format::R8_UINT;
	GPU::Handle textureHandle = GPU::Manager::CreateTexture(textureDesc, initialDataSubRsc, testName.c_str());

	GPU::TextureDesc readbackDesc;
	readbackDesc.type_ = GPU::TextureType::TEX2D;
	readbackDesc.bindFlags_ = GPU::BindFlags::NONE;
	readbackDesc.width_ = width;
	readbackDesc.height_ = height;
	readbackDesc.levels_ = levels;
	readbackDesc.format_ = GPU::Format::R8_UINT;
	GPU::Handle readbackHandle = GPU::Manager::CreateTexture(readbackDesc, nullptr, testName.c_str());

	GPU::Handle fenceHandle = GPU::Manager::CreateFence(0, testName.c_str());

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	auto commandListHandle = GPU::Manager::CreateCommandList(testName.c_str());

	SECTION("copy all - readback all")
	{
		GPU::CommandList commandList(GPU::CommandList::DEFAULT_BUFFER_SIZE);

		GPU::Point destPoint = {};
		destPoint.x_ = 0;
		destPoint.y_ = 0;
		destPoint.z_ = 0;

		GPU::Box srcBox = {};
		srcBox.x_ = 0;
		srcBox.y_ = 0;
		srcBox.z_ = 0;
		srcBox.w_ = width;
		srcBox.h_ = height;
		srcBox.d_ = 1;

		REQUIRE(commandList.CopyTextureSubResource(readbackHandle, 0, destPoint, textureHandle, 0, srcBox));

		srcBox.w_ = width / 2;
		srcBox.h_ = height / 2;
		srcBox.d_ = 1;
		REQUIRE(commandList.CopyTextureSubResource(readbackHandle, 1, destPoint, textureHandle, 1, srcBox));

		REQUIRE(GPU::Manager::CompileCommandList(commandListHandle, commandList));

		REQUIRE(GPU::Manager::SubmitCommandList(commandListHandle));

		REQUIRE(GPU::Manager::SubmitFence(fenceHandle, 1));
		REQUIRE(GPU::Manager::WaitOnFence(fenceHandle, 1));

		Core::Vector<u8> readbackData((i32)totalBytes);
		GPU::TextureSubResourceData readbackDataSubRsc = {};
		readbackDataSubRsc.data_ = readbackData.data();
		readbackDataSubRsc.rowPitch_ = width;
		readbackDataSubRsc.slicePitch_ = width * height;
		REQUIRE(GPU::Manager::ReadbackTextureSubresource(readbackHandle, 0, readbackDataSubRsc));

		REQUIRE(memcmp(initialData.data(), readbackData.data(), readbackDataSubRsc.slicePitch_) == 0);

		readbackDataSubRsc.data_ = readbackData.data() + readbackDataSubRsc.slicePitch_;
		readbackDataSubRsc.rowPitch_ = width / 2;
		readbackDataSubRsc.slicePitch_ = (width / 2) * (height / 2);
		REQUIRE(GPU::Manager::ReadbackTextureSubresource(readbackHandle, 1, readbackDataSubRsc));

		REQUIRE(memcmp(initialData.data(), readbackData.data(), totalBytes) == 0);
	}

	GPU::Manager::DestroyResource(commandListHandle);
	GPU::Manager::DestroyResource(fenceHandle);
	GPU::Manager::DestroyResource(readbackHandle);
	GPU::Manager::DestroyResource(textureHandle);
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
