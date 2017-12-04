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
		static const i16 MAJOR_VERSION = 0x0002;
		/// Mimor version signifies non-breaking change to binary format.
		static const i16 MINOR_VERSION = 0x0000;

		u32 magic_ = MAGIC;
		i16 majorVersion_ = MAJOR_VERSION;
		i16 minorVersion_ = MINOR_VERSION;

		i32 numCBuffers_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numSamplers_ = 0;
		i32 numShaders_ = 0;
		i32 numTechniques_ = 0;
		i32 numSamplerStates_ = 0;
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
		char name_[MAX_NAME_LENGTH] = {'\0'};
		i32 vs_ = -1;
		i32 gs_ = -1;
		i32 hs_ = -1;
		i32 ds_ = -1;
		i32 ps_ = -1;
		i32 cs_ = -1;
		GPU::RenderState rs_; // TODO: Store separately.
	};

	struct ShaderSamplerStateHeader
	{
		char name_[MAX_NAME_LENGTH] = {'\0'};
		GPU::SamplerState state_;
	};

	struct ShaderImpl
	{
		Core::String name_;
		Graphics::ShaderHeader header_;
		Core::Vector<ShaderBindingHeader> bindingHeaders_;
		Core::Vector<ShaderBytecodeHeader> bytecodeHeaders_;
		Core::Vector<ShaderBindingMapping> bindingMappings_;
		Core::Vector<ShaderTechniqueHeader> techniqueHeaders_;
		Core::Vector<ShaderSamplerStateHeader> samplerStateHeaders_;
		Core::Vector<u8> bytecode_;

		Core::Vector<GPU::Handle> samplerStates_;
		Core::Vector<GPU::Handle> shaders_;
		Core::Vector<const ShaderBindingMapping*> shaderBindingMappings_;

		// All technique impls currently active.
		Core::Vector<ShaderTechniqueImpl*> techniques_;

		// Data that's between different techniques.
		Core::Vector<u32> techniqueDescHashes_;
		Core::Vector<ShaderTechniqueDesc> techniqueDescs_;
		Core::Vector<GPU::Handle> pipelineStates_;

		Job::RWLock rwLock_;

		ShaderImpl();
		~ShaderImpl();

		/// Get binding index from name.
		i32 GetBindingIndex(const char* name) const;

		/// Get binding name from index.
		const char* GetBindingName(i32 idx) const;

		/// Create technique to match @a name and @a desc.
		ShaderTechniqueImpl* CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

		/// Setup @a impl to reference the currently loaded shader appropriately.
		bool SetupTechnique(ShaderTechniqueImpl* impl);
	};

	struct ShaderTechniqueImpl
	{
		ShaderImpl* shader_ = nullptr;
		ShaderTechniqueHeader header_;
		i32 descIdx_ = -1;

		bool bsDirty_ = true;
		GPU::PipelineBindingSetDesc bs_;
		GPU::Handle bsHandle_;

		i32 cbvOffset_ = 0;
		i32 srvOffset_ = 0;
		i32 uavOffset_ = 0;
		i32 samplerOffset_ = 0;
		i32 maxBindings = 0;

		Core::Vector<GPU::BindingBuffer> cbvs_;
		Core::Vector<GPU::BindingSRV> srvs_;
		Core::Vector<GPU::BindingUAV> uavs_;
		Core::Vector<GPU::BindingSampler> samplers_;

		void Invalidate()
		{
			header_.vs_ = -1;
			header_.gs_ = -1;
			header_.hs_ = -1;
			header_.ds_ = -1;
			header_.ps_ = -1;
			header_.cs_ = -1;
			bsDirty_ = true;
		}

		bool IsValid() const
		{
			return header_.vs_ != -1 || header_.gs_ != -1 || header_.hs_ != -1 || header_.ds_ != -1 ||
			       header_.ps_ != -1 || header_.cs_ != -1;
		};
	};

} // namespace Graphics
