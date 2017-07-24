#pragma once
#include "core/hash.h"
#include "core/set.h"
#include "core/string.h"
#include "core/vector.h"
#include "gpu/resources.h"
#include "graphics/shader.h"
#include "job/concurrency.h"

namespace Graphics
{
	static const i32 MAX_NAME_LENGTH = 64;

	struct ShaderHeader
	{
		/// Magic number.
		static const u32 MAGIC = 0x229C08ED;
		/// Major version signifies a breaking change to the binary format.
		static const i16 MAJOR_VERSION = 0x0000;
		/// Mimor version signifies non-breaking change to binary format.
		static const i16 MINOR_VERSION = 0x0003;

		u32 magic_ = MAGIC;
		i16 majorVersion_ = MAJOR_VERSION;
		i16 minorVersion_ = MINOR_VERSION;

		i32 numCBuffers_ = 0;
		i32 numSamplers_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numShaders_ = 0;
		i32 numTechniques_ = 0;
	};

	struct ShaderBindingHeader
	{
		char name_[MAX_NAME_LENGTH];
	};

	struct ShaderBytecodeHeader
	{
		i32 numCBuffers_ = 0;
		i32 numSamplers_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		GPU::ShaderType type_ = GPU::ShaderType::INVALID;
		i32 offset_ = 0;
		i32 numBytes_ = 0;
	};

	struct ShaderBindingMapping
	{
		i32 binding_ = 0;
		i32 dstSlot_ = 0;
	};

	struct ShaderTechniqueHeader
	{
		char name_[MAX_NAME_LENGTH];
		i32 vs_;
		i32 gs_;
		i32 hs_;
		i32 ds_;
		i32 ps_;
		i32 cs_;
		GPU::RenderState rs_; // TODO: Store separately.
	};

	struct ShaderImpl
	{
		Core::String name_;
		Graphics::ShaderHeader header_;
		Core::Vector<ShaderBindingHeader> bindingHeaders_;
		Core::Vector<ShaderBytecodeHeader> bytecodeHeaders_;
		Core::Vector<ShaderBindingMapping> bindingMappings_;
		Core::Vector<ShaderTechniqueHeader> techniqueHeaders_;
		Core::Vector<u8> bytecode_;

		Core::Vector<GPU::Handle> shaders_;
		Core::Vector<const ShaderBindingMapping*> shaderBindingMappings_;

		// Technique data.
		Core::Vector<u32> techniqueDescHashes_;
		Core::Vector<ShaderTechniqueDesc> techniqueDescs_; //
		Core::Vector<GPU::Handle> pipelineStates_;

		Core::Vector<ShaderTechniqueImpl*> techniques_;

		Job::SpinLock reloadLock_;


		ShaderImpl();
		~ShaderImpl();
		ShaderTechniqueImpl* CreateTechnique(
		    const char* name, const ShaderTechniqueDesc& desc, ShaderTechniqueImpl* impl);
	};

	struct ShaderTechniqueImpl
	{
		ShaderImpl* shader_ = nullptr;
		const ShaderTechniqueHeader* header_ = nullptr;
		i32 descIdx_ = 0;

		bool bsDirty_ = true;
		GPU::PipelineBindingSetDesc bs_;
		GPU::Handle bsHandle_;

		i32 samplerOffset_ = 0;
		i32 srvOffset_ = 0;
		i32 uavOffset_ = 0;
		i32 maxBindings = 0;

		Core::Vector<GPU::BindingBuffer> cbvs_;
		Core::Vector<GPU::BindingSampler> samplers_;
		Core::Vector<GPU::BindingSRV> srvs_;
		Core::Vector<GPU::BindingUAV> uavs_;

		void Invalidate() { header_ = nullptr; }
	};

} // namespace Graphics
