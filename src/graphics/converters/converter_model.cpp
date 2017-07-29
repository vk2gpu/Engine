#include "graphics/model.h"
#include "graphics/private/model_impl.h"
#include "core/file.h"
#include "core/half.h"
#include "core/misc.h"
#include "core/pair.h"
#include "gpu/enum.h"
#include "gpu/utils.h"
#include "math/aabb.h"
#include "math/mat44.h"
#include "resource/converter.h"
#include "serialization/serializer.h"


#include "assimp/config.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

#include <cstring>

namespace
{
	class BinaryStream
	{
	public:
		BinaryStream() {}

		~BinaryStream() {}

		const i32 GROW_ALIGNMENT = 1024 * 1024;

		void GrowAmount(i32 amount)
		{
			i32 minSize = Core::PotRoundUp(offset_ + amount, GROW_ALIGNMENT);
			if(minSize > data_.size())
				data_.resize(minSize);
		}

		void Write(const void* data, i32 bytes)
		{
			GrowAmount(bytes);
			DBG_ASSERT(offset_ + bytes <= data_.size());
			memcpy(data_.data() + offset_, data, bytes);
			offset_ += bytes;
		}

		template<typename TYPE>
		void Write(const TYPE& data)
		{
			Write(&data, sizeof(data));
		}

		const void* Data() const { return data_.data(); }
		i32 Size() const { return offset_; }

	private:
		i32 offset_ = 0;
		Core::Vector<u8> data_;
	};

	bool FloatToVertexDataType(f32* inFloats, GPU::Format format, void* outData, i32& outBytes)
	{
		DBG_ASSERT(inFloats != nullptr);

		const auto formatInfo = GPU::GetFormatInfo(format);
		i32 noofFloats = 0;
		if(formatInfo.rBits_ > 0)
			++noofFloats;
		if(formatInfo.gBits_ > 0)
			++noofFloats;
		if(formatInfo.bBits_ > 0)
			++noofFloats;
		if(formatInfo.aBits_ > 0)
			++noofFloats;

		switch(noofFloats)
		{
		case 1:
			DBG_ASSERT(formatInfo.gBits_ == 0);
			DBG_ASSERT(formatInfo.bBits_ == 0);
			DBG_ASSERT(formatInfo.aBits_ == 0);
			break;
		case 2:
			DBG_ASSERT(formatInfo.rBits_ > 0);
			DBG_ASSERT(formatInfo.gBits_ == formatInfo.rBits_);
			DBG_ASSERT(formatInfo.bBits_ == 0);
			DBG_ASSERT(formatInfo.aBits_ == 0);
			break;
		case 3:
			DBG_ASSERT(formatInfo.rBits_ > 0);
			DBG_ASSERT(formatInfo.gBits_ == formatInfo.rBits_);
			DBG_ASSERT(formatInfo.bBits_ == formatInfo.rBits_);
			DBG_ASSERT(formatInfo.aBits_ == 0);
			break;
		case 4:
			DBG_ASSERT(formatInfo.rBits_ > 0);
			DBG_ASSERT(formatInfo.gBits_ == formatInfo.rBits_);
			DBG_ASSERT(formatInfo.bBits_ == formatInfo.rBits_);
			DBG_ASSERT(formatInfo.aBits_ == formatInfo.rBits_);
			break;
		default:
			return false;
			break;
		};

		const auto bits = formatInfo.rBits_;

		if(outData == nullptr)
			return true;

		// Calculate total output size.
		u8* outDataBytes = reinterpret_cast<u8*>(outData);
		outBytes = (bits * noofFloats) / 8;

#define INTEGER_ELEMENT(_bits, _dataType)                                                                              \
                                                                                                                       \
	if(bits == _bits)                                                                                                  \
		for(i32 idx = 0; idx < noofFloats; ++idx)                                                                      \
		{                                                                                                              \
			*reinterpret_cast<_dataType*>(outDataBytes) = Core::Clamp(static_cast<_dataType>(*inFloats++),             \
			    std::numeric_limits<_dataType>::min(), std::numeric_limits<_dataType>::max());                         \
			outDataBytes += (bits / 8);                                                                                \
		}

#define NORM_ELEMENT(_bits, _dataType)                                                                                 \
	if(bits == _bits)                                                                                                  \
		for(i32 idx = 0; idx < noofFloats; ++idx)                                                                      \
		{                                                                                                              \
			*reinterpret_cast<_dataType*>(outDataBytes) =                                                              \
			    Core::Clamp(static_cast<_dataType>(*inFloats++ * std::numeric_limits<_dataType>::max()),               \
			        std::numeric_limits<_dataType>::min(), std::numeric_limits<_dataType>::max());                     \
			outDataBytes += (bits / 8);                                                                                \
		}

		if(formatInfo.rgbaFormat_ == GPU::FormatType::FLOAT)
		{
			if(bits == 32)
			{
				memcpy(outData, inFloats, outBytes);
			}
			else if(bits == 16)
			{
				Core::FloatToHalf(inFloats, (u16*)outDataBytes, noofFloats);
			}
		}
		else if(formatInfo.rgbaFormat_ == GPU::FormatType::UINT)
		{
			INTEGER_ELEMENT(8, u8);
			INTEGER_ELEMENT(16, u16);
			INTEGER_ELEMENT(32, u32);
		}
		else if(formatInfo.rgbaFormat_ == GPU::FormatType::SINT)
		{
			INTEGER_ELEMENT(8, i8);
			INTEGER_ELEMENT(16, i16);
			INTEGER_ELEMENT(32, i32);
		}
		else if(formatInfo.rgbaFormat_ == GPU::FormatType::UNORM)
		{
			NORM_ELEMENT(8, u8);
			NORM_ELEMENT(16, u16);
			NORM_ELEMENT(32, u32);
		}
		else if(formatInfo.rgbaFormat_ == GPU::FormatType::SNORM)
		{
			NORM_ELEMENT(8, i8);
			NORM_ELEMENT(16, i16);
			NORM_ELEMENT(32, i32);
		}
		else
		{
			return false;
		}

#undef INTEGER_ELEMENT
#undef NORM_ELEMENT
		return true;
	}
}

namespace
{
	/**
	 * Assimp logging function.
	 */
	void AssimpLogStream(const char* message, char* user)
	{
		if(strstr(message, "Error") != nullptr || strstr(message, "Warning") != nullptr)
		{
			DBG_LOG("ASSIMP: %s", message);
		}
	}

	/**
	 * Determine material name.
	 */
	std::string AssimpGetMaterialName(aiMaterial* material)
	{
		aiString aiName("default");
		// Try material name.
		if(material->Get(AI_MATKEY_NAME, aiName) == aiReturn_SUCCESS)
		{
		}
		// Try diffuse texture.
		else if(material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), aiName) == aiReturn_SUCCESS)
		{
		}
		return aiName.C_Str();
	}

	/**
	 * Fill next element that is less than zero.
	 * Will check elements until first one less than 0.0 is found and overwrite it.
	 */
	i32 FillNextElementLessThanZero(f32 value, f32* elements, i32 noofElements)
	{
		for(i32 idx = 0; idx < noofElements; ++idx)
		{
			if(elements[idx] < 0.0f)
			{
				elements[idx] = value;
				return idx;
			}
		}

		return -1;
	}

	/**
	 * Fill all elements less than zero with specific value.
	 */
	void FillAllElementsLessThanZero(f32 value, f32* elements, i32 noofElements)
	{
		for(i32 idx = 0; idx < noofElements; ++idx)
		{
			if(elements[idx] < 0.0f)
			{
				elements[idx] = value;
			}
		}
	}
}

namespace
{
	class ConverterModel : public Resource::IConverter
	{
	public:
		ConverterModel() {}

		virtual ~ConverterModel() {}

		struct MetaData
		{
			bool isInitialized_ = false;
			bool flattenHierarchy_ = false;
			i32 maxBones_ = 256;
			i32 maxBoneInfluences_ = 4;

			struct
			{
				GPU::Format position_ = GPU::Format::R32G32B32_FLOAT;
				GPU::Format normal_ = GPU::Format::R16G16B16A16_SNORM;
				GPU::Format texcoord_ = GPU::Format::R16G16_FLOAT;
				GPU::Format color_ = GPU::Format::R8G8B8A8_UNORM;
			} vertexFormat_;

			bool Serialize(Serialization::Serializer& serializer)
			{
				isInitialized_ = true;

				bool retVal = true;
				retVal &= serializer.Serialize("flattenHierarchy", flattenHierarchy_);
				retVal &= serializer.Serialize("maxBones", maxBones_);
				retVal &= serializer.Serialize("maxBoneInfluences", maxBoneInfluences_);
				if(auto object = serializer.Object("vertexFormat"))
				{
					retVal &= serializer.Serialize("position", vertexFormat_.position_);
					retVal &= serializer.Serialize("normal", vertexFormat_.normal_);
					retVal &= serializer.Serialize("texcoord", vertexFormat_.texcoord_);
					retVal &= serializer.Serialize("color", vertexFormat_.color_);
				}
				return retVal;
			}
		};

		struct MeshData
		{
			GPU::PrimitiveTopology primTopology_ = GPU::PrimitiveTopology::TRIANGLE_LIST;
			i32 noofVertices_ = 0;
			i32 noofIndices_ = 0;
			i32 indexStride_ = 0;
			i32 idx_ = -1;
			Core::Vector<GPU::VertexElement> elements_;
			Core::Vector<Graphics::ModelMeshDraw> draws_;
			BinaryStream vertexData_;
			BinaryStream indexData_;
		};

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Model::GetTypeUUID()) ||
			       (fileExt &&
			           (strcmp(fileExt, "obj") == 0 || strcmp(fileExt, "fbx") == 0 || strcmp(fileExt, "gltf") == 0));
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			metaData_ = context.GetMetaData<MetaData>();

			char resolvedSourcePath[Core::MAX_PATH_LENGTH];
			memset(resolvedSourcePath, 0, sizeof(resolvedSourcePath));
			if(!context.GetPathResolver()->ResolvePath(sourceFile, resolvedSourcePath, sizeof(resolvedSourcePath)))
				return false;
			char file[Core::MAX_PATH_LENGTH];
			char path[Core::MAX_PATH_LENGTH];
			memset(file, 0, sizeof(file));
			memset(path, 0, sizeof(path));
			if(!Core::FileSplitPath(resolvedSourcePath, path, sizeof(path), file, sizeof(file), nullptr, 0))
			{
				context.AddError(__FILE__, __LINE__, "INTERNAL ERROR: Core::FileSplitPath failed.");
				return false;
			}

			char outFilename[Core::MAX_PATH_LENGTH];
			memset(outFilename, 0, sizeof(outFilename));
			strcat_s(outFilename, sizeof(outFilename), destPath);
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);

			bool retVal = false;

			auto propertyStore = aiCreatePropertyStore();
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBBC_MAX_BONES, metaData_.maxBones_);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_LBW_MAX_WEIGHTS, metaData_.maxBoneInfluences_);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_MD5_NO_ANIM_AUTOLOAD, true);

			aiLogStream assimpLogger = {AssimpLogStream, (char*)this};
			aiAttachLogStream(&assimpLogger);

			// TODO: Intercept file io to track dependencies.
			int flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_SplitByBoneCount |
			            aiProcess_LimitBoneWeights | aiProcess_ConvertToLeftHanded;
			if(metaData_.flattenHierarchy_)
			{
				flags |= aiProcess_OptimizeGraph | aiProcess_RemoveComponent;
				aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS,
				    aiComponent_ANIMATIONS | aiComponent_LIGHTS | aiComponent_CAMERAS);
			}

			scene_ = aiImportFileExWithProperties(resolvedSourcePath, flags, nullptr, propertyStore);

			aiReleasePropertyStore(propertyStore);

			if(scene_ == nullptr)
				return false;

			i32 nodeIdx = 0;
			i32 meshIdx = 0;
			RecursiveSerialiseNodes(scene_->mRootNode, -1, nodeIdx, meshIdx);
			aiReleaseImport(scene_);
			scene_ = nullptr;

			CalculateNodeWorldTransforms();

			// Write out converted model.
			Core::File outFile(outFilename, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
			if(outFile)
			{
				// Model data header.
				Graphics::ModelData modelData;
				modelData.numNodes_ = nodes_.size();
				modelData.numMeshNodes_ = meshNodes_.size();
				modelData.numMeshes_ = meshIdx;
				modelData.numAABBs_ = meshNodeAABBDatas_.size();
				modelData.numBonePalettes_ = meshNodeBonePaletteDatas_.size();
				modelData.numInverseBindPoses_ = meshNodeInverseBindposeDatas_.size();
				modelData.numMaterials_ = 0;
				outFile.Write(&modelData, sizeof(modelData));

				// Local, world, and parent indices for nodes.
				for(i32 idx = 0; idx < nodes_.size(); ++idx)
					outFile.Write(&nodes_[idx].local_, sizeof(Math::Mat44));
				for(i32 idx = 0; idx < nodes_.size(); ++idx)
					outFile.Write(&nodes_[idx].world_, sizeof(Math::Mat44));
				for(i32 idx = 0; idx < nodes_.size(); ++idx)
					outFile.Write(&nodes_[idx].parent_, sizeof(i32));

				// Model node mesh.
				for(const auto& data : meshNodes_)
					outFile.Write(&data, sizeof(Graphics::MeshNode));

				for(const auto& data : meshNodeAABBDatas_)
					outFile.Write(&data, sizeof(Graphics::MeshNodeAABB));

				for(const auto& data : meshNodeBonePaletteDatas_)
					outFile.Write(data.second, data.first * sizeof(Graphics::MeshNodeBonePalette));

				for(const auto& data : meshNodeInverseBindposeDatas_)
					outFile.Write(data.second, data.first * sizeof(Graphics::MeshNodeInverseBindpose));

				// TODO: materials.

				// Write out mesh data.
				i32 numVertexElements = 0;
				i32 numDraws = 0;
				for(const auto& mesh : meshDatas_)
				{
					Graphics::ModelMeshData meshData;

					// Calculate output vertex size.
					u32 stride = 0;
					for(const auto& element : mesh.elements_)
					{
						u32 size = GPU::GetFormatInfo(element.format_).blockBits_ / 8;
						stride = Core::Max(stride, element.offset_ + size);
					}

					meshData.noofVertices_ = mesh.noofVertices_;
					meshData.vertexStride_ = stride;
					meshData.noofIndices_ = mesh.noofIndices_;
					meshData.indexStride_ = mesh.indexStride_;
					meshData.startVertexElements_ = numVertexElements;
					meshData.endVertexElements_ = numVertexElements + mesh.elements_.size();
					meshData.startDraws_ = numDraws;
					meshData.endDraws_ = numDraws + mesh.draws_.size();
					outFile.Write(&meshData, sizeof(meshData));

					numVertexElements += mesh.elements_.size();
					numDraws += mesh.draws_.size();
				}

				// Vertex elements.
				for(const auto& mesh : meshDatas_)
				{
					for(const auto& element : mesh.elements_)
					{
						outFile.Write(&element, sizeof(element));
					}
				}

				// Draws.
				for(const auto& mesh : meshDatas_)
				{
					for(const auto& draw : mesh.draws_)
					{
						outFile.Write(&draw, sizeof(draw));
					}
				}

				// Write out mesh vertex + index buffers.
				for(const auto& mesh : meshDatas_)
				{
					outFile.Write(mesh.vertexData_.Data(), mesh.vertexData_.Size());
				}
				for(const auto& mesh : meshDatas_)
				{
					outFile.Write(mesh.indexData_.Data(), mesh.indexData_.Size());
				}

				retVal = true;
			}


			if(retVal)
			{
				context.AddOutput(outFilename);
			}

			// Setup metadata.
			context.SetMetaData(metaData_);

			return retVal;
		}

		void RecursiveSerialiseNodes(aiNode* node, i32 parentIdx, i32& nodeIdx, i32& meshIdx)
		{
			// Setup structs.
			Graphics::NodeDataAoS nodeData;
			nodeData.parent_ = parentIdx;
			nodeData.local_ = Math::Mat44((const f32*)&node->mTransformation).Transposed();
			nodeData.world_ = Math::Mat44();

			nodes_.push_back(nodeData);

			parentIdx = nodeIdx++;

			// Setup mesh data.
			if(node->mNumMeshes > 0)
			{
				for(size_t idx = 0; idx < node->mNumMeshes; ++idx)
				{
					SerialiseMesh(scene_->mMeshes[node->mMeshes[idx]], parentIdx, nodeIdx, meshIdx);
				}
			}

			// Recurse into children.
			for(size_t idx = 0; idx < node->mNumChildren; ++idx)
			{
				RecursiveSerialiseNodes(node->mChildren[idx], parentIdx, nodeIdx, meshIdx);
			}
		}

		void SerialiseMesh(struct aiMesh* mesh, i32 parentIdx, i32& nodeIdx, i32& meshIdx)
		{
			if(mesh->HasPositions() && mesh->HasFaces())
			{
				// Calculate number of primitives.
				DBG_ASSERT(Core::BitsSet(mesh->mPrimitiveTypes) == 1);

				Core::Array<GPU::VertexElement, GPU::MAX_VERTEX_ELEMENTS> elements;
				i32 numElements = 0;

				// Vertex format.
				elements[numElements++] =
				    GPU::VertexElement(0, 0, metaData_.vertexFormat_.position_, GPU::VertexUsage::POSITION, 0);

				if(mesh->HasNormals())
				{
					elements[numElements++] =
					    GPU::VertexElement(0, 0, metaData_.vertexFormat_.normal_, GPU::VertexUsage::NORMAL, 0);
				}

				if(mesh->HasTangentsAndBitangents())
				{
					elements[numElements++] =
					    GPU::VertexElement(0, 0, metaData_.vertexFormat_.normal_, GPU::VertexUsage::TANGENT, 0);

					elements[numElements++] =
					    GPU::VertexElement(0, 0, metaData_.vertexFormat_.normal_, GPU::VertexUsage::BINORMAL, 0);
				}

				for(i32 idx = 0; idx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++idx)
				{
					if(mesh->HasTextureCoords(idx) || idx == 0)
					{
						elements[numElements++] = GPU::VertexElement(
						    0, 0, metaData_.vertexFormat_.texcoord_, GPU::VertexUsage::TEXCOORD, idx);
					}
				}

				for(i32 idx = 0; idx < AI_MAX_NUMBER_OF_COLOR_SETS; ++idx)
				{
					if(mesh->HasVertexColors(idx) || idx == 0)
					{
						elements[numElements++] =
						    GPU::VertexElement(0, 0, metaData_.vertexFormat_.color_, GPU::VertexUsage::COLOR, idx);
					}
				}

				// Add bones to vertex declaration if they exist.
				if(mesh->HasBones())
				{
					const i32 numBoneVectors = Core::PotRoundUp(metaData_.maxBoneInfluences_, 4) / 4;

					for(i32 idx = 0; idx < numBoneVectors; ++idx)
					{
						if(mesh->HasVertexColors(idx))
						{
							elements[numElements++] = GPU::VertexElement(
							    0, 0, GPU::Format::R8G8B8A8_UINT, GPU::VertexUsage::BLENDINDICES, idx);
						}
					}

					for(i32 idx = 0; idx < numBoneVectors; ++idx)
					{
						if(mesh->HasVertexColors(idx))
						{
							elements[numElements++] = GPU::VertexElement(
							    0, 0, GPU::Format::R16G16B16A16_UNORM, GPU::VertexUsage::BLENDWEIGHTS, idx);
						}
					}
				}

				// Calculate offsets per-stream.
				Core::Array<i32, GPU::MAX_VERTEX_STREAMS> offsets;
				offsets.fill(0);
				for(i32 elementIdx = 0; elementIdx < numElements; ++elementIdx)
				{
					auto& element(elements[elementIdx]);
					i32 size = GPU::GetFormatInfo(element.format_).blockBits_ / 8;
					element.offset_ = offsets[element.streamIdx_];
					offsets[element.streamIdx_] += size;
				}

				GPU::PrimitiveTopology primTopology = GPU::PrimitiveTopology::TRIANGLE_LIST;

				// Grab primitive type.
				switch(mesh->mPrimitiveTypes)
				{
				case aiPrimitiveType_POINT:
					primTopology = GPU::PrimitiveTopology::POINT_LIST;
					break;
				case aiPrimitiveType_LINE:
					primTopology = GPU::PrimitiveTopology::LINE_LIST;
					break;
				case aiPrimitiveType_TRIANGLE:
					primTopology = GPU::PrimitiveTopology::TRIANGLE_LIST;
					break;
				default:
					DBG_BREAK;
				}

				// Get packed mesh.
				MeshData& meshData = GetMeshData(primTopology, elements.data(), numElements, 2);

				Graphics::MeshNode meshNode;
				Graphics::MeshNodeAABB modelMeshAABB;
				Graphics::MeshNodeBonePalette* modelMeshBonePalette = nullptr;
				Graphics::MeshNodeInverseBindpose* modelMeshInverseBindPose = nullptr;
				meshNode.meshIdx_ = meshData.idx_;
				meshNode.noofBones_ = mesh->mNumBones;

				if(meshNode.noofBones_ > 0)
				{
					modelMeshBonePalette = Graphics::MeshNodeBonePalette::Create(mesh->mNumBones);
					modelMeshInverseBindPose = Graphics::MeshNodeInverseBindpose::Create(mesh->mNumBones);

					meshNode.bonePaletteIdx_ = meshNodeBonePaletteDatas_.size();
					meshNode.inverseBindPoseIdx_ = meshNodeInverseBindposeDatas_.size();

					meshNodeBonePaletteDatas_.emplace_back(mesh->mNumBones, modelMeshBonePalette);
					meshNodeInverseBindposeDatas_.emplace_back(mesh->mNumBones, modelMeshInverseBindPose);

					for(i32 boneidx = 0; boneidx < (i32)mesh->mNumBones; ++boneidx)
					{
						const auto* bone = mesh->mBones[boneidx];
						i32 nodeBaseIndex = 0;
						modelMeshBonePalette->indices_[boneidx] =
						    FindNodeIndex(bone->mName.C_Str(), scene_->mRootNode, nodeBaseIndex);
						modelMeshInverseBindPose->transforms_[boneidx] =
						    Math::Mat44(bone->mOffsetMatrix[0]).Transposed();
					}
				}

				// Setup draw for this bit of the mesh.
				Graphics::ModelMeshDraw draw;

				// Export vertices.
				draw.vertexOffset_ = meshData.noofVertices_;
				draw.noofVertices_ =
				    SerialiseVertices(mesh, elements.data(), numElements, modelMeshAABB.aabb_, meshData.vertexData_);
				meshData.noofVertices_ += draw.noofVertices_;

				// Export indices.
				draw.indexOffset_ = meshData.noofIndices_;
				draw.noofIndices_ = SerialiseIndices(mesh, meshData.indexData_);
				meshData.noofIndices_ += draw.noofIndices_;

				// Push AABB.
				meshNode.aabbIdx_ = meshNodeAABBDatas_.size();
				meshNodeAABBDatas_.push_back(modelMeshAABB);

				// Add draw to render mesh.
				meshData.draws_.push_back(draw);
				meshNode.drawIdx_ = meshData.draws_.size() - 1;

				// Add model node data.
				meshNodes_.push_back(meshNode);

				// Grab material name.
				//Core::String materialName;
				//aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];

				// Import material.
				//MeshData.MaterialRef_ = findMaterialMatch(Material);
				//MeshData_.push_back(MeshData);

				// Update primitive index.
				++meshIdx;
			}
		}

		u32 SerialiseVertices(struct aiMesh* mesh, GPU::VertexElement* elements, size_t numElements, Math::AABB& aabb,
		    BinaryStream& stream) const
		{
			aabb.Empty();

			// Build blend weights and indices.
			Core::Vector<Math::Vec4> blendWeights;
			Core::Vector<Math::Vec4> blendIndices;

			const i32 numBoneVectors = Core::PotRoundUp(metaData_.maxBoneInfluences_, 4) / 4;
			if(mesh->HasBones())
			{
				// Clear off to less than zero to signify empty.
				blendWeights.resize(mesh->mNumVertices * numBoneVectors, Math::Vec4(-1.0f, -1.0f, -1.0f, -1.0f));
				blendIndices.resize(mesh->mNumVertices * numBoneVectors, Math::Vec4(-1.0f, -1.0f, -1.0f, -1.0f));

				// Populate the weights and indices.
				for(u32 boneidx = 0; boneidx < mesh->mNumBones; ++boneidx)
				{
					auto* bone = mesh->mBones[boneidx];
					for(u32 weightidx = 0; weightidx < bone->mNumWeights; ++weightidx)
					{
						const auto& WeightVertex = bone->mWeights[weightidx];

						Math::Vec4& blendWeight = blendWeights[WeightVertex.mVertexId * numBoneVectors];
						Math::Vec4& blendIndex = blendIndices[WeightVertex.mVertexId * numBoneVectors];

						u32 blendWeightElementidx = FillNextElementLessThanZero(
						    WeightVertex.mWeight, reinterpret_cast<f32*>(&blendWeight), numBoneVectors * 4);
						u32 blendIndexElementidx = FillNextElementLessThanZero(
						    (f32)(boneidx), reinterpret_cast<f32*>(&blendIndex), numBoneVectors * 4);
						DBG_ASSERT(blendWeightElementidx == blendIndexElementidx);
					}
				}

				// Fill the rest of the weights and indices with valid, but empty values.
				for(u32 Vertidx = 0; Vertidx < mesh->mNumVertices; ++Vertidx)
				{
					Math::Vec4& blendWeight = blendWeights[Vertidx];
					Math::Vec4& blendIndex = blendIndices[Vertidx];

					FillAllElementsLessThanZero(0.0f, reinterpret_cast<f32*>(&blendWeight), numBoneVectors * 4);
					FillAllElementsLessThanZero(0.0f, reinterpret_cast<f32*>(&blendIndex), numBoneVectors * 4);
				}
			}

			// Calculate output vertex stride.
			u32 stride = 0;
			for(u32 elementIdx = 0; elementIdx < numElements; ++elementIdx)
			{
				const auto& element(elements[elementIdx]);
				u32 size = GPU::GetFormatInfo(element.format_).blockBits_ / 8;
				stride = Core::Max(stride, element.offset_ + size);
			}

			Core::Vector<u8> vertexData(stride, 0);

			for(u32 vertexidx = 0; vertexidx < mesh->mNumVertices; ++vertexidx)
			{
				aiVector3D position =
				    mesh->mVertices != nullptr ? mesh->mVertices[vertexidx] : aiVector3D(0.0f, 0.0f, 0.0f);

				// Expand aabb.
				aabb.ExpandBy(Math::Vec3(position.x, position.y, position.z));

				for(u32 elementIdx = 0; elementIdx < numElements; ++elementIdx)
				{
					const auto& element(elements[elementIdx]);

					switch(element.usage_)
					{
					case GPU::VertexUsage::POSITION:
					{
						f32 input[] = {position.x, position.y, position.z, 1.0f};
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::NORMAL:
					{
						f32 input[] = {0.0f, 0.0f, 0.0f, 0.0f};
						if(mesh->mNormals != nullptr)
						{
							input[0] = mesh->mNormals[vertexidx].x;
							input[1] = mesh->mNormals[vertexidx].y;
							input[2] = mesh->mNormals[vertexidx].z;
						}
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::TANGENT:
					{
						f32 input[] = {0.0f, 0.0f, 0.0f, 0.0f};
						if(mesh->mTangents != nullptr)
						{
							input[0] = mesh->mTangents[vertexidx].x;
							input[1] = mesh->mTangents[vertexidx].y;
							input[2] = mesh->mTangents[vertexidx].z;
						}
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::BINORMAL:
					{
						f32 input[] = {0.0f, 0.0f, 0.0f, 0.0f};
						if(mesh->mBitangents != nullptr)
						{
							input[0] = mesh->mBitangents[vertexidx].x;
							input[1] = mesh->mBitangents[vertexidx].y;
							input[2] = mesh->mBitangents[vertexidx].z;
						}
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::TEXCOORD:
					{
						DBG_ASSERT(element.usageIdx_ < AI_MAX_NUMBER_OF_TEXTURECOORDS);
						f32 input[] = {0.0f, 0.0f, 0.0f, 0.0f};
						if(mesh->mTextureCoords[element.usageIdx_] != nullptr)
						{
							input[0] = mesh->mTextureCoords[element.usageIdx_][vertexidx].x;
							input[1] = mesh->mTextureCoords[element.usageIdx_][vertexidx].y;
							input[2] = mesh->mTextureCoords[element.usageIdx_][vertexidx].z;
						}
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::COLOR:
					{
						DBG_ASSERT(element.usageIdx_ < AI_MAX_NUMBER_OF_COLOR_SETS);
						f32 input[] = {1.0f, 1.0f, 1.0f, 1.0f};
						if(mesh->mColors[element.usageIdx_] != nullptr)
						{
							input[0] = mesh->mColors[element.usageIdx_][vertexidx].r;
							input[1] = mesh->mColors[element.usageIdx_][vertexidx].g;
							input[2] = mesh->mColors[element.usageIdx_][vertexidx].b;
							input[3] = mesh->mColors[element.usageIdx_][vertexidx].a;
						}
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::BLENDINDICES:
					{
						f32 input[] = {blendIndices[vertexidx + (element.usageIdx_ * numBoneVectors)].x,
						    blendIndices[vertexidx + (element.usageIdx_ * numBoneVectors)].y,
						    blendIndices[vertexidx + (element.usageIdx_ * numBoneVectors)].z,
						    blendIndices[vertexidx + (element.usageIdx_ * numBoneVectors)].w};
						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;
					case GPU::VertexUsage::BLENDWEIGHTS:
					{
						f32 input[] = {blendWeights[vertexidx + (element.usageIdx_ * numBoneVectors)].x,
						    blendWeights[vertexidx + (element.usageIdx_ * numBoneVectors)].y,
						    blendWeights[vertexidx + (element.usageIdx_ * numBoneVectors)].z,
						    blendWeights[vertexidx + (element.usageIdx_ * numBoneVectors)].w};

						i32 outSize = 0;
						FloatToVertexDataType(input, element.format_, &vertexData[element.offset_], outSize);
					}
					break;

					default:
						break;
					};
				}

				stream.Write(vertexData.data(), vertexData.size());
			}
			return mesh->mNumVertices;
		}

		i32 SerialiseIndices(struct aiMesh* mesh, BinaryStream& stream) const
		{
			i32 totalIndices = 0;
			for(i32 faceidx = 0; faceidx < (i32)mesh->mNumFaces; ++faceidx)
			{
				const auto& face = mesh->mFaces[faceidx];
				for(i32 indexidx = 0; indexidx < (i32)face.mNumIndices; ++indexidx)
				{
					i32 index = face.mIndices[indexidx];
					DBG_ASSERT(index < 0x10000);
					stream.Write(u16(index));
					++totalIndices;
				}
			}
			return totalIndices;
		}

		MeshData& GetMeshData(
		    GPU::PrimitiveTopology primTopology, GPU::VertexElement* elements, i32 numElements, i32 indexStride)
		{
			auto it = std::find_if(meshDatas_.begin(), meshDatas_.end(), [&](const MeshData& meshData) {
				bool declMatches = false;
				if(meshData.elements_.size() == numElements)
				{
					declMatches = true;
					for(i32 idx = 0; idx < numElements; ++idx)
					{
						declMatches &=
						    memcmp(&meshData.elements_[idx], &elements[idx], sizeof(GPU::VertexElement)) == 0;
					}
				}

				return declMatches && meshData.primTopology_ == primTopology && meshData.indexStride_ == indexStride;
			});
			if(it == meshDatas_.end())
			{
				MeshData meshData;
				meshData.primTopology_ = primTopology;
				meshData.elements_.insert(elements, elements + numElements);
				meshData.indexStride_ = indexStride;
				meshData.idx_ = meshDatas_.size();
				it = meshDatas_.emplace_back(meshData);
			}
			return *it;
		}

		i32 FindNodeIndex(const char* name, aiNode* root, i32& baseIndex) const
		{
			if(strcmp(root->mName.C_Str(), name) == 0)
			{
				return baseIndex;
			}

			i32 foundIndex = -1;
			for(i32 idx = 0; idx < (i32)root->mNumChildren; ++idx)
			{
				auto child = root->mChildren[idx];
				++baseIndex;
				foundIndex = FindNodeIndex(name, child, baseIndex);

				if(foundIndex != -1)
				{
					return foundIndex;
				}
			}

			return foundIndex;
		}

		void CalculateNodeWorldTransforms()
		{
			for(i32 idx = 0; idx < nodes_.size(); ++idx)
			{
				auto& nodeData(nodes_[idx]);
				if(nodeData.parent_ != -1)
				{
					DBG_ASSERT(nodeData.parent_ < idx);
					const auto& parentNodeData(nodes_[nodeData.parent_]);
					nodeData.world_ = nodeData.local_ * parentNodeData.world_;
				}
				else
				{
					nodeData.world_ = nodeData.local_;
				}
			}
		}

		MetaData metaData_;
		const aiScene* scene_ = nullptr;
		Core::Vector<Graphics::NodeDataAoS> nodes_;
		Core::Vector<Graphics::MeshNode> meshNodes_;
		Core::Vector<Graphics::MeshNodeAABB> meshNodeAABBDatas_;
		Core::Vector<Core::Pair<i32, Graphics::MeshNodeBonePalette*>> meshNodeBonePaletteDatas_;
		Core::Vector<Core::Pair<i32, Graphics::MeshNodeInverseBindpose*>> meshNodeInverseBindposeDatas_;
		Core::Vector<MeshData> meshDatas_;
	};
}

extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = Resource::ConverterPlugin::PLUGIN_VERSION;
			outPlugin->uuid_ = Resource::ConverterPlugin::GetUUID();
			outPlugin->name_ = "Graphics.Model Converter";
			outPlugin->desc_ = "Model converter plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
			plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterModel(); };
			plugin->DestroyConverter = [](Resource::IConverter*& converter) {
				delete converter;
				converter = nullptr;
			};
		}
		retVal = true;
	}

	return retVal;
}
}
