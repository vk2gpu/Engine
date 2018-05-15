#include "catch.hpp"
#include "test_shared.h"

#include "core/misc.h"
#include "core/random.h"
#include "core/string.h"
#include "graphics/virtual_texture.h"
#include "graphics/private/virtual_texture_impl.h"
#include "image/color.h"
#include "image/image.h"
#include "image/load.h"
#include "image/process.h"
#include "image/save.h"

namespace
{
	using Point = Graphics::VTPoint;
	using Rect = Graphics::VTRect;

	Rect Divide(Rect r, i32 div) { return {r.x / div, r.y / div, r.w / div, r.h / div}; };

	bool GetTexel(Image::SRGBAColor& outColor, const Image::Image& image, i32 x, i32 y, i32 l = 0)
	{
		if(x < image.GetWidth() && y < image.GetHeight())
		{
			const auto* texels = image.GetMipData<Image::SRGBAColor>(l);
			i32 h = std::max(image.GetHeight() >> l, 1);
			outColor = texels[x + y * h];
			return true;
		}
		return false;
	}

	bool SetTexel(Image::Image& image, const Image::SRGBAColor inColor, i32 x, i32 y, i32 l = 0)
	{
		if(x < image.GetWidth() && y < image.GetHeight())
		{
			auto* texels = image.GetMipData<Image::SRGBAColor>(l);
			i32 h = std::max(image.GetHeight() >> l, 1);
			texels[x + y * h] = inColor;
			return true;
		}
		return false;
	}

	bool Fill(Image::Image& dst, Rect dstRect, const Image::SRGBAColor color)
	{
		dstRect.x = Core::Clamp(dstRect.x, 0, dst.GetWidth() - 1);
		dstRect.y = Core::Clamp(dstRect.y, 0, dst.GetHeight() - 1);
		dstRect.w = Core::Clamp(dstRect.w, 0, dst.GetWidth() - dstRect.x);
		dstRect.h = Core::Clamp(dstRect.h, 0, dst.GetHeight() - dstRect.y);
		for(i32 y = dstRect.y; y < (dstRect.h + dstRect.y); ++y)
		{
			for(i32 x = dstRect.x; x < (dstRect.w + dstRect.x); ++x)
			{
				SetTexel(dst, color, x, y);
			}
		}
		return true;
	}

	bool Blit(Image::Image& dst, Point dstPoint, i32 dstLevel, const Image::Image& src, Rect srcRect, i32 srcLevel)
	{
		if(dst.GetFormat() != src.GetFormat())
			return false;

		for(i32 y = 0; y < srcRect.h; ++y)
		{
			for(i32 x = 0; x < srcRect.w; ++x)
			{
				Image::SRGBAColor color;
				if(GetTexel(color, src, x + srcRect.x, y + srcRect.y, srcLevel))
				{
					SetTexel(dst, color, x + dstPoint.x, y + dstPoint.y, dstLevel);
				}
			}
		}
		return true;
	}

	Image::Image ReadbackTexture(GPU::TextureDesc desc, GPU::Handle src)
	{
		DBG_ASSERT(desc.type_ == GPU::TextureType::TEX2D);

		desc.bindFlags_ = GPU::BindFlags::NONE;
		GPU::Handle readback = GPU::Manager::CreateTexture(desc, nullptr, "Readback");
		GPU::Handle compiledCmdList = GPU::Manager::CreateCommandList("Readback");
		GPU::Handle fence = GPU::Manager::CreateFence(0, "Readback");
		GPU::CommandList cmdList;

		GPU::Point dstPoint = {0, 0};
		GPU::Box srcBox = {0, 0, 0, desc.width_, desc.height_, desc.depth_};

		for(i32 l = 0; l < desc.levels_; ++l)
		{
			cmdList.CopyTextureSubResource(readback, l, dstPoint, src, l, srcBox);

			srcBox.w_ = std::max(srcBox.w_ / 2, 1);
			srcBox.h_ = std::max(srcBox.h_ / 2, 1);
			srcBox.d_ = std::max(srcBox.d_ / 2, 1);
		}

		GPU::Manager::CompileCommandList(compiledCmdList, cmdList);
		GPU::Manager::SubmitCommandList(compiledCmdList);
		GPU::Manager::SubmitFence(fence, 1);
		GPU::Manager::WaitOnFence(fence, 1);

		Image::Image image(
		    desc.type_, desc.format_, desc.width_, desc.height_, desc.depth_, desc.levels_, nullptr, nullptr);

		for(i32 l = 0; l < desc.levels_; ++l)
		{
			auto footprint = GPU::GetTextureFootprint(desc.format_, desc.width_, desc.height_);
			GPU::TextureSubResourceData texSubRsc;
			texSubRsc.data_ = image.GetMipBaseAddr(l);
			texSubRsc.rowPitch_ = footprint.rowPitch_;
			texSubRsc.slicePitch_ = footprint.slicePitch_;
			GPU::Manager::ReadbackTextureSubresource(readback, l, texSubRsc);

			desc.width_ = std::max<i32>(desc.width_ / 2, 1);
			desc.height_ = std::max<i32>(desc.height_ / 2, 1);
			desc.depth_ = std::max<i16>(desc.depth_ / 2, 1);
		}

		GPU::Manager::DestroyResource(readback);
		GPU::Manager::DestroyResource(compiledCmdList);
		GPU::Manager::DestroyResource(fence);

		return image;
	}
}

TEST_CASE("graphics-tests-virtual-texture-allocation")
{
	ScopedEngine engine;

	const i32 vtDim = 16 * 1024;
	const i32 pageDim = 128;
	const i32 tabDim = vtDim / pageDim;

	Graphics::VTAllocator pageManager(vtDim, pageDim, 0);

	auto testImage = Image::Image(
	    Image::ImageType::TEX2D, Image::ImageFormat::R8G8B8A8_UNORM, tabDim, tabDim, 1, 1, nullptr, nullptr);

	Graphics::VTNodeAllocation alloc = {};

	for(i32 i = 0; i < 4; ++i)
	{
		auto col = (u8)(16 + ((i % 16) * 8));
		alloc = pageManager.AllocPages(4096, 4096);
		Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(col, 0, 0, 255));
	}

	for(i32 i = 0; i < 8; ++i)
	{
		auto col = (u8)(16 + ((i % 16) * 8));
		alloc = pageManager.AllocPages(1024, 1024);
		Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(0, col, 0, 255));
	}

	for(i32 i = 0; i < 256; ++i)
	{
		auto col = (u8)(16 + ((i % 16) * 8));
		alloc = pageManager.AllocPages(256, 256);
		Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(0, 0, col, 255));
	}

	for(i32 i = 0; i < 512; ++i)
	{
		auto col = (u8)(16 + ((i % 16) * 8));
		alloc = pageManager.AllocPages(128, 128);
		Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(0, col, col, 255));
	}

	if(auto file = Core::File("texture-allocation.png", Core::FileFlags::DEFAULT_WRITE))
	{
		Image::Save(file, testImage, Image::FileType::PNG);
	}
}

TEST_CASE("graphics-tests-virtual-texture-allocation-randomized")
{
	ScopedEngine engine;

	const i32 vtDim = 256 * 1024;
	const i32 pageDim = 256;
	const i32 tabDim = vtDim / pageDim;

	Graphics::VTAllocator pageManager(vtDim, pageDim, 0);

	auto testImage = Image::Image(
	    Image::ImageType::TEX2D, Image::ImageFormat::R8G8B8A8_UNORM, tabDim, tabDim, 1, 1, nullptr, nullptr);

	Core::Vector<Graphics::VTNodeAllocation> allocs;

	Core::Random rng;
	Graphics::VTNodeAllocation alloc = {};

	auto size = 0;

	for(i32 i = 0; i < (32 * 1024); ++i)
	{
		auto col = (u8)(16 + ((i % 16) * 8));

		alloc = pageManager.AllocPages(1 << (8 + (size * 2)), 1 << (8 + (size * 2)));

		if(alloc.node_)
		{
			allocs.emplace_back(alloc);

			switch(size)
			{
			case 0:
				Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(col, 0, 0, 255));
				break;
			case 1:
				Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(0, col, 0, 255));
				break;
			case 2:
				Fill(testImage, Divide(alloc.node_->rect_, pageDim), Image::SRGBAColor(0, 0, col, 255));
				break;
			};

			size = (u32)rng.Generate() % 3;
		}
		else
		{
			auto ri = (u32)rng.Generate() % allocs.size();

			Fill(testImage, Divide(allocs[ri].node_->rect_, pageDim), Image::SRGBAColor(255, 0, 255, 32));
			pageManager.FreePages(allocs[ri]);
			allocs.erase(&allocs[ri]);
		}

		if((i % 3) == 0 && i > 256)
		{
			auto ri = (u32)rng.Generate() % allocs.size();

			Fill(testImage, Divide(allocs[ri].node_->rect_, pageDim), Image::SRGBAColor(255, 0, 255, 32));
			pageManager.FreePages(allocs[ri]);
			allocs.erase(&allocs[ri]);
		}

		CHECK(allocs.size() == pageManager.GetTotalAllocs());
	}

	REQUIRE(allocs.size() == pageManager.GetTotalAllocs());

	if(auto file = Core::File("texture-allocation-randomized.png", Core::FileFlags::DEFAULT_WRITE))
	{
		Image::Save(file, testImage, Image::FileType::PNG);
	}
}

TEST_CASE("graphics-tests-virtual-texture-vtmanager")
{
	ScopedEngine engine;

	static const i32 vtDim = 8 * 1024;
	static const i32 pageDim = 256;
	static const i32 maxResident = 128;

	static const GPU::Format formats[] = {GPU::Format::R8G8B8A8_UNORM};

	GPU::Manager::ScopedDebugCapture capture("virtual-texturing");

	struct TestProvider : public Graphics::IVTPageProvider
	{
		Core::Vector<Image::Image> images_;

		GPU::Handle dstTex_;
		GPU::Footprint dstFootprint_;
		GPU::CommandList cmdList_;
		GPU::Handle compiledCmdList_;

		struct Request
		{
			i32 idx = 0;
			i32 level = 0;
			Graphics::VTPoint dst = {};
			Graphics::VTRect src = {};
		};

		Core::Vector<Request> requests_;

		TestProvider()
		{
			const char* images[] = {
			    "../../../../res/model_tests/crytek-sponza/textures_pbr/Background_Albedo.png",
			    "../../../../res/model_tests/crytek-sponza/textures_pbr/ChainTexture_Albedo.png",
			    "../../../../res/model_tests/crytek-sponza/textures_pbr/Lion_Albedo.png",
			    "../../../../res/model_tests/crytek-sponza/textures_pbr/Sponza_Fabric_Red_diffuse.png",
			};

			for(const char* imageName : images)
			{
				// Load all images and generate mipmaps.
				if(auto file = Core::File(imageName, Core::FileFlags::DEFAULT_READ))
				{
					Image::Image image = Image::Load(file);

					const i32 levels =
					    32 - Core::CountLeadingZeros((u32)Core::Max(image.GetWidth(), image.GetHeight()));

					Image::Image lsImage = Image::Image(image.GetType(), Image::ImageFormat::R32G32B32A32_FLOAT,
					    image.GetWidth(), image.GetHeight(), image.GetDepth(), levels, nullptr, nullptr);
					Image::Image newImage;

					bool success = true;

					// Unpack to floating point.
					success &= Image::Convert(lsImage, image, Image::ImageFormat::R32G32B32A32_FLOAT);
					DBG_ASSERT(success);

					// Convert to linear space.
					success &= Image::GammaToLinear(lsImage, lsImage);
					DBG_ASSERT(success);

					// Generate mips.
					success &= Image::GenerateMips(lsImage, lsImage);
					DBG_ASSERT(success);

					// Convert to gamma space.
					success &= Image::LinearToGamma(lsImage, lsImage);
					DBG_ASSERT(success);

					// Pack back to 32-bit RGBA.
					success &= Image::Convert(newImage, lsImage, Image::ImageFormat::R8G8B8A8_UNORM);
					DBG_ASSERT(success);

					images_.emplace_back(std::move(newImage));
				}
			}

			compiledCmdList_ = GPU::Manager::CreateCommandList("TestProvider Command List");

			// hack
			const i32 size = 12 * pageDim;
			//dstImage_ = Image::Image(Image::ImageType::TEX2D, formats[0], size, size, 1, 1, nullptr, nullptr);
		}

		~TestProvider() { GPU::Manager::DestroyResource(compiledCmdList_); }

		bool RequestPage(i32 idx, i32 level, Graphics::VTPoint dstPoint, Graphics::VTRect srcRect) override
		{
			requests_.emplace_back(Request{idx, level, dstPoint, srcRect});

			return true;
		}

		void FlushRequests(Graphics::VTIndirection& ind)
		{
			if(requests_.size() > 0)
			{
				if(auto ev = cmdList_.Event(0x0, "FlushRequests"))
				{
					if(auto ev2 = cmdList_.Event(0x0, "Update Cache"))
					{
						for(const auto& req : requests_)
						{
							const auto& srcImage = images_[req.idx];
							const i32 w = Core::Max(1, srcImage.GetWidth() >> req.level);
							const i32 h = Core::Max(1, srcImage.GetHeight() >> req.level);

							GPU::Footprint footprint = GPU::GetTextureFootprint(srcImage.GetFormat(), w, h, 1);

							GPU::Point dst = {req.dst.x, req.dst.y, 0};
							GPU::Box src = {req.src.x, req.src.y, 0, req.src.w, req.src.h, 1};
							cmdList_.UpdateTextureSubResource(dstTex_, 0, dst, src, 
								srcImage.GetMipBaseAddr(req.level),
								footprint);
						}
					}

					if(auto ev3 = cmdList_.Event(0x0, "Update Indirection"))
					{
						ind.FlushIndirection(cmdList_);
					}
				}

				GPU::Manager::CompileCommandList(compiledCmdList_, cmdList_);
				cmdList_.Reset();

				GPU::Manager::SubmitCommandList(compiledCmdList_);
			}
		}
	};

	TestProvider provider;

	Graphics::VTManager mgr(vtDim, pageDim, maxResident, formats, &provider);

	const i32 texDim = mgr.numPagesDim_ * pageDim;
	provider.dstTex_ = mgr.textures_[0];
	provider.dstFootprint_ = GPU::GetTextureFootprint(mgr.formats_[0], texDim, texDim);

	Core::Vector<i32> textures;
	for(const auto& image : provider.images_)
	{
		textures.push_back(mgr.CreateTexture(image.GetWidth(), image.GetHeight()));
	}

	mgr.RequestPages();

	// Flush all requests to GPU.
	provider.FlushRequests(mgr.indirection_);

	struct TestParams
	{
		Math::Vec2 texOffset_;
		Math::Vec2 texScale_;
		u32 texID_;
	};

	TestParams testParams;
	testParams.texID_ = 0;
	auto texAlloc = mgr.GetAllocation(testParams.texID_);
	auto texRect = texAlloc.node_->rect_;
	testParams.texOffset_ = Math::Vec2((f32)texRect.x, (f32)texRect.y) / (f32)mgr.allocator_.vtDim_;
	testParams.texScale_ = Math::Vec2((f32)texRect.w, (f32)texRect.h) / (f32)mgr.allocator_.vtDim_;

	GPU::BufferDesc bufDesc;
	bufDesc.size_ = sizeof(TestParams);
	bufDesc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
	auto cb = GPU::Manager::CreateBuffer(bufDesc, &testParams, "testParams");

	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX2D;
	texDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	texDesc.width_ = texRect.w;
	texDesc.height_ = texRect.h;

	auto ua = GPU::Manager::CreateTexture(texDesc, nullptr, "outTex");

	Graphics::ShaderRef testShader = "shaders/vt_test.esf";
	testShader.WaitUntilReady();

	auto testBindingSet = testShader->CreateBindingSet("VTTestBindings");
	testBindingSet.Set("testParams", GPU::Binding::ConstantBuffer(cb, 0, (i32)bufDesc.size_));
	testBindingSet.Set("outTex", GPU::Binding::RWTexture2D(ua, GPU::Format::R8G8B8A8_UNORM));

	struct VTParams
	{
		Math::Vec2 tileSize_;
		Math::Vec2 vtSize_;
		Math::Vec2 cacheSize_;
		i32 feedbackDivisor_;
	};

	VTParams vtParams;
	vtParams.tileSize_ = Math::Vec2((f32)pageDim, (f32)pageDim);
	vtParams.vtSize_ = Math::Vec2((f32)vtDim, (f32)vtDim);
	vtParams.cacheSize_ = Math::Vec2((f32)mgr.numPagesDim_, (f32)mgr.numPagesDim_) * vtParams.tileSize_;
	vtParams.feedbackDivisor_ = 4;

	GPU::BufferDesc paramsDesc;
	paramsDesc.size_ = sizeof(VTParams);
	paramsDesc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
	auto paramsHandle = GPU::Manager::CreateBuffer(paramsDesc, &vtParams, "VTParams");

	auto vtBindingSet = testShader->CreateBindingSet("VTBindings");
	vtBindingSet.Set(
	    "inVTIndirection", GPU::Binding::Texture2D(mgr.indirection_.tex_, GPU::Format::R8G8B8A8_UINT, 0, -1));
	vtBindingSet.Set("vtParams", GPU::Binding::ConstantBuffer(paramsHandle, 0, sizeof(VTParams)));
	vtBindingSet.Set("inVTCache", GPU::Binding::Texture2D(mgr.textures_[0], GPU::Format::R8G8B8A8_UNORM, 0, 1));
	/// dummy...
	vtBindingSet.Set("outVTFeedback", GPU::Binding::RWTexture2D(ua, GPU::Format::R8G8B8A8_UNORM));

	auto tech = testShader->CreateTechnique("TestIndirection", Graphics::ShaderTechniqueDesc());

	GPU::CommandList cmdList;
	Graphics::ShaderContext shaderCtx(cmdList);

	shaderCtx.SetBindingSet(vtBindingSet);
	shaderCtx.SetBindingSet(testBindingSet);

	GPU::Handle ps;
	Core::ArrayView<GPU::PipelineBinding> pb;
	shaderCtx.CommitBindings(tech, ps, pb);

	cmdList.Dispatch(ps, pb, texDesc.width_ / 8, texDesc.height_ / 8, 1);

	auto fence = GPU::Manager::CreateFence(0, "Fence");

	auto compiledCmdList = GPU::Manager::CreateCommandList("Indirection Command List");
	GPU::Manager::CompileCommandList(compiledCmdList, cmdList);
	GPU::Manager::SubmitCommandList(compiledCmdList);
	GPU::Manager::SubmitFence(fence, 1);
	GPU::Manager::WaitOnFence(fence, 1);

	if(auto file = Core::File("readback-texture-cache.png", Core::FileFlags::DEFAULT_WRITE))
	{
		auto image = ReadbackTexture(mgr.textureDescs_[0], mgr.textures_[0]);
		Image::Save(file, image, Image::FileType::PNG);
	}

	if(auto file = Core::File("readback-texture-indirection.png", Core::FileFlags::DEFAULT_WRITE))
	{
		auto image = ReadbackTexture(mgr.indirection_.texDesc_, mgr.indirection_.tex_);
		Image::Save(file, image, Image::FileType::PNG);
	}

	if(auto file = Core::File("readback-texture-rendered.png", Core::FileFlags::DEFAULT_WRITE))
	{
		auto image = ReadbackTexture(texDesc, ua);
		Image::Save(file, image, Image::FileType::PNG);

		auto psnr = Image::CalculatePSNR(image, provider.images_[testParams.texID_]);

		// Should be exact comparison.
		REQUIRE(psnr.r == Image::INFINITE_PSNR);
		REQUIRE(psnr.g == Image::INFINITE_PSNR);
		REQUIRE(psnr.b == Image::INFINITE_PSNR);
		REQUIRE(psnr.a == Image::INFINITE_PSNR);
	}

	GPU::Manager::DestroyResource(paramsHandle);
	GPU::Manager::DestroyResource(cb);
	GPU::Manager::DestroyResource(ua);
	GPU::Manager::DestroyResource(fence);
	GPU::Manager::DestroyResource(compiledCmdList);
}
