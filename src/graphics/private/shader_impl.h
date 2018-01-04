#pragma once
#include "core/hash.h"
#include "core/set.h"
#include "core/string.h"
#include "core/vector.h"
#include "gpu/resources.h"
#include "graphics/shader.h"
#include "job/concurrency.h"

namespace Serialization
{
	class Serializer;
} // namespace Serialization

namespace Graphics
{
	static const i32 MAX_NAME_LENGTH = 64;
	static const i32 MAX_BOUND_BINDING_SETS = 16;

	enum class ShaderBindingFlags : u32
	{
		INVALID = 0x0000,

		CBV = 0x1000,
		SRV = 0x2000,
		UAV = 0x4000,
		SAMPLER = 0x8000,

		TYPE_MASK = 0xf000,
		INDEX_MASK = 0x0fff,
	};


	DEFINE_ENUM_CLASS_FLAG_OPERATOR(ShaderBindingFlags, |);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(ShaderBindingFlags, &);

	struct GRAPHICS_DLL ShaderHeader
	{
		/// Magic number.
		static const u32 MAGIC = 0x229C08ED;
		/// Major version signifies a breaking change to the binary format.
		static const i16 MAJOR_VERSION = 0x0004;
		/// Minor version signifies non-breaking change to binary format.
		static const i16 MINOR_VERSION = 0x0000;

		u32 magic_ = MAGIC;
		i16 majorVersion_ = MAJOR_VERSION;
		i16 minorVersion_ = MINOR_VERSION;

		i32 numShaders_ = 0;
		i32 numTechniques_ = 0;
		i32 numSamplerStates_ = 0;

		i32 numBindingSets_ = 0;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderBindingSetHeader
	{
		char name_[MAX_NAME_LENGTH] = {'\0'};
		bool isShared_ = false;
		i32 frequency_ = 0;
		i32 numCBVs_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numSamplers_ = 0;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderBindingHeader
	{
		char name_[MAX_NAME_LENGTH] = {'\0'};
		ShaderBindingHandle handle_;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderBytecodeHeader
	{
		GPU::ShaderType type_ = GPU::ShaderType::INVALID;
		i32 offset_ = 0;
		i32 numBytes_ = 0;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderTechniqueBindingSlot
	{
		i32 idx_ = -1;
		i32 cbvReg_ = 0;
		i32 srvReg_ = 0;
		i32 uavReg_ = 0;
		i32 samplerReg_ = 0;
	};

	struct GRAPHICS_DLL ShaderTechniqueHeader
	{
		char name_[MAX_NAME_LENGTH] = {'\0'};
		i32 vs_ = -1;
		i32 hs_ = -1;
		i32 ds_ = -1;
		i32 gs_ = -1;
		i32 ps_ = -1;
		i32 cs_ = -1;
		GPU::RenderState rs_; // TODO: Store separately.

		i32 numBindingSets_ = 0;
		Core::Array<ShaderTechniqueBindingSlot, MAX_BOUND_BINDING_SETS> bindingSlots_;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderSamplerStateHeader
	{
		char name_[MAX_NAME_LENGTH] = {'\0'};
		GPU::SamplerState state_;

		bool Serialize(Serialization::Serializer& serializer);
	};

	struct GRAPHICS_DLL ShaderImpl
	{
		Core::String name_;
		Graphics::ShaderHeader header_;
		Core::Vector<ShaderBindingSetHeader> bindingSetHeaders_;
		Core::Vector<ShaderBindingHeader> bindingHeaders_;
		Core::Vector<ShaderBytecodeHeader> bytecodeHeaders_;
		Core::Vector<ShaderTechniqueHeader> techniqueHeaders_;
		Core::Vector<ShaderSamplerStateHeader> samplerStateHeaders_;
		Core::Vector<u8> bytecode_;

		Core::Vector<GPU::Handle> samplerStates_;
		Core::Vector<GPU::Handle> shaders_;

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

		/// Create binding set.
		ShaderBindingSetImpl* CreateBindingSet(const char* name);

		/// Setup @a impl to reference the currently loaded shader appropriately.
		bool SetupTechnique(ShaderTechniqueImpl* impl);
	};

	struct ShaderTechniqueImpl
	{
		ShaderImpl* shader_ = nullptr;
		ShaderTechniqueHeader header_;
		i32 descIdx_ = -1;

		void Invalidate()
		{
			header_.vs_ = -1;
			header_.gs_ = -1;
			header_.hs_ = -1;
			header_.ds_ = -1;
			header_.ps_ = -1;
			header_.cs_ = -1;
		}

		bool IsValid() const
		{
			return header_.vs_ != -1 || header_.gs_ != -1 || header_.hs_ != -1 || header_.ds_ != -1 ||
			       header_.ps_ != -1 || header_.cs_ != -1;
		};
	};

	struct ShaderBindingSetImpl
	{
		ShaderBindingSetHeader header_;
		i32 idx_ = -1;

		GPU::Handle pbs_;

		Core::Vector<GPU::BindingCBV> cbvs_;
		Core::Vector<GPU::BindingSRV> srvs_;
		Core::Vector<GPU::BindingUAV> uavs_;
		Core::Vector<GPU::SamplerState> samplers_;
	};

	struct ShaderContextImpl
	{
		ShaderContextImpl(GPU::CommandList& cmdList)
		    : cmdList_(cmdList)
		{
		}

		GPU::CommandList& cmdList_;
		Core::Vector<ShaderBindingSetImpl*> bindingSets_;

#if !defined(FINAL)
		struct Callstack
		{
			Core::Array<void*, 32> fns_ = {};
			i32 hash_ = 0;
		};

		Core::Vector<Callstack> bindingCallstacks_;
#endif // !defined(FINAL)
	};

} // namespace Graphics
