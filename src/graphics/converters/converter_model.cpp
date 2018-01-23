#include "graphics/model.h"
#include "graphics/converters/import_model.h"
#include "graphics/private/model_impl.h"
#include "core/concurrency.h"
#include "core/file.h"
#include "core/half.h"
#include "core/misc.h"
#include "core/pair.h"
#include "core/type_conversion.h"
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
#include <regex>
#include <limits>

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

	bool GetInStreamDesc(Core::StreamDesc& outDesc, GPU::VertexUsage usage)
	{
		switch(usage)
		{
		case GPU::VertexUsage::POSITION:
		case GPU::VertexUsage::NORMAL:
		case GPU::VertexUsage::TEXCOORD:
		case GPU::VertexUsage::TANGENT:
		case GPU::VertexUsage::BINORMAL:
			outDesc.dataType_ = Core::DataType::FLOAT;
			outDesc.numBits_ = 32;
			outDesc.stride_ = 3 * sizeof(f32);
			break;
		case GPU::VertexUsage::BLENDWEIGHTS:
		case GPU::VertexUsage::BLENDINDICES:
		case GPU::VertexUsage::COLOR:
			outDesc.dataType_ = Core::DataType::FLOAT;
			outDesc.numBits_ = 32;
			outDesc.stride_ = 4 * sizeof(f32);
			break;
		default:
			return false;
		}
		return outDesc.numBits_ > 0;
	}

	bool GetOutStreamDesc(Core::StreamDesc& outDesc, GPU::Format format)
	{
		auto formatInfo = GPU::GetFormatInfo(format);
		outDesc.dataType_ = formatInfo.rgbaFormat_;
		outDesc.numBits_ = formatInfo.rBits_;
		outDesc.stride_ = formatInfo.blockBits_ >> 3;
		return outDesc.numBits_ > 0;
	}
}

namespace
{
	Core::Mutex assimpMutex_;

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
	Core::String AssimpGetMaterialName(aiMaterial* material)
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

		using VertexBinaryStreams = Core::Array<BinaryStream, GPU::MAX_VERTEX_STREAMS>;
		struct MeshData
		{
			GPU::PrimitiveTopology primTopology_ = GPU::PrimitiveTopology::TRIANGLE_LIST;
			i32 noofVertices_ = 0;
			i32 noofIndices_ = 0;
			i32 indexStride_ = 0;
			i32 idx_ = -1;
			Core::Vector<GPU::VertexElement> elements_;
			Core::Vector<Graphics::ModelMeshDraw> draws_;
			VertexBinaryStreams vertexData_;
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
			context_ = &context;
			metaData_ = context.GetMetaData<Graphics::MetaDataModel>();

			// Setup default 'catch all' material.
			if(metaData_.isInitialized_ == false)
			{
				Graphics::MetaDataModel::Material material;
				material.regex_ = "(.*)";
				material.template_.shader_ = "shaders/default.esf";
				metaData_.materials_.push_back(material);
			}
			std::reverse(metaData_.materials_.begin(), metaData_.materials_.end());

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

			sourceFile_ = resolvedSourcePath;

			char outFilename[Core::MAX_PATH_LENGTH];
			memset(outFilename, 0, sizeof(outFilename));
			strcat_s(outFilename, sizeof(outFilename), destPath);
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);

			bool retVal = false;

			auto propertyStore = aiCreatePropertyStore();
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBBC_MAX_BONES, metaData_.maxBones_);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_LBW_MAX_WEIGHTS, metaData_.maxBoneInfluences_);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_MD5_NO_ANIM_AUTOLOAD, true);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_ICL_PTCACHE_SIZE, 64);
			aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65534);
			aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, metaData_.smoothingAngle_);

			aiLogStream assimpLogger = {AssimpLogStream, (char*)this};
			{
				Core::ScopedMutex lock(assimpMutex_);
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
				aiDetachLogStream(&assimpLogger);
			}

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
				modelData.numMeshes_ = meshDatas_.size();
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

				// Write out mesh data.
				i32 numVertexElements = 0;
				i32 numDraws = 0;
				for(const auto& mesh : meshDatas_)
				{
					Graphics::ModelMeshData meshData;

					// Calculate output vertex size.
					u32 vertexSize = 0;
					for(const auto& element : mesh.elements_)
					{
						u32 size = GPU::GetFormatInfo(element.format_).blockBits_ / 8;
						vertexSize += size;
					}

					meshData.noofVertices_ = mesh.noofVertices_;
					meshData.vertexSize_ = vertexSize;
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
					for(const auto& stream : mesh.vertexData_)
						if(stream.Size() > 0)
							outFile.Write(stream.Data(), stream.Size());
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
			std::reverse(metaData_.materials_.begin(), metaData_.materials_.end());
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

			// Setup mesh data.
			if(node->mNumMeshes > 0)
			{
				for(size_t idx = 0; idx < node->mNumMeshes; ++idx)
				{
					SerialiseMesh(scene_->mMeshes[node->mMeshes[idx]], parentIdx, nodeIdx, meshIdx);
				}
			}

			parentIdx = nodeIdx++;

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

				i32 currStream = 0;

				// Vertex format.
				elements[numElements++] =
				    GPU::VertexElement(currStream, 0, metaData_.vertexFormat_.position_, GPU::VertexUsage::POSITION, 0);

				if(metaData_.splitStreams_)
					currStream++;

				if(mesh->HasNormals())
				{
					elements[numElements++] =
					    GPU::VertexElement(currStream, 0, metaData_.vertexFormat_.normal_, GPU::VertexUsage::NORMAL, 0);

					if(metaData_.splitStreams_)
						currStream++;
				}

				if(mesh->HasTangentsAndBitangents() && metaData_.vertexFormat_.tangent_ != GPU::Format::INVALID)
				{
					elements[numElements++] = GPU::VertexElement(
					    currStream, 0, metaData_.vertexFormat_.tangent_, GPU::VertexUsage::TANGENT, 0);

					if(metaData_.splitStreams_)
						currStream++;
				}

				for(i32 idx = 0; idx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++idx)
				{
					if(mesh->HasTextureCoords(idx))
					{
						elements[numElements++] = GPU::VertexElement(
						    currStream, 0, metaData_.vertexFormat_.texcoord_, GPU::VertexUsage::TEXCOORD, idx);

						if(metaData_.splitStreams_)
							currStream++;
					}
				}

				for(i32 idx = 0; idx < AI_MAX_NUMBER_OF_COLOR_SETS; ++idx)
				{
					if(mesh->HasVertexColors(idx))
					{
						elements[numElements++] = GPU::VertexElement(
						    currStream, 0, metaData_.vertexFormat_.color_, GPU::VertexUsage::COLOR, idx);

						if(metaData_.splitStreams_)
							currStream++;
					}
				}

				// Add bones to vertex declaration if they exist.
				if(mesh->HasBones())
				{
					const i32 numBoneVectors = Core::PotRoundUp(metaData_.maxBoneInfluences_, 4) / 4;

					for(i32 idx = 0; idx < numBoneVectors; ++idx)
					{
						elements[numElements++] =
						    GPU::VertexElement(4, 0, GPU::Format::R8G8B8A8_UINT, GPU::VertexUsage::BLENDINDICES, idx);
					}

					for(i32 idx = 0; idx < numBoneVectors; ++idx)
					{
						elements[numElements++] =
						    GPU::VertexElement(4, 0, GPU::Format::R8G8B8A8_UNORM, GPU::VertexUsage::BLENDWEIGHTS, idx);
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
					DBG_ASSERT(false);
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

				// Setup node.
				meshNode.nodeIdx_ = nodeIdx;

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

				// Grab material name.
				Core::String materialName;
				aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];

				// Import material.
				meshNode.material_ = AddMaterial(material);

				// Add model node data.
				meshNodes_.push_back(meshNode);

				// Update primitive index.
				++meshIdx;
			}
		}

		u32 SerialiseVertices(struct aiMesh* mesh, GPU::VertexElement* elements, i32 numElements, Math::AABB& aabb,
		    VertexBinaryStreams& streams) const
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
				for(i32 boneidx = 0; boneidx < (i32)mesh->mNumBones; ++boneidx)
				{
					auto* bone = mesh->mBones[boneidx];
					for(i32 weightidx = 0; weightidx < (i32)bone->mNumWeights; ++weightidx)
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
				for(i32 Vertidx = 0; Vertidx < (i32)mesh->mNumVertices; ++Vertidx)
				{
					Math::Vec4& blendWeight = blendWeights[Vertidx];
					Math::Vec4& blendIndex = blendIndices[Vertidx];

					FillAllElementsLessThanZero(0.0f, reinterpret_cast<f32*>(&blendWeight), numBoneVectors * 4);
					FillAllElementsLessThanZero(0.0f, reinterpret_cast<f32*>(&blendIndex), numBoneVectors * 4);
				}
			}

			// Calculate AABB.
			for(i32 vertexidx = 0; vertexidx < (i32)mesh->mNumVertices; ++vertexidx)
			{
				aiVector3D position =
				    mesh->mVertices != nullptr ? mesh->mVertices[vertexidx] : aiVector3D(0.0f, 0.0f, 0.0f);

				// Expand aabb.
				aabb.ExpandBy(Math::Vec3(position.x, position.y, position.z));
			}

			for(i32 vtxStreamIdx = 0; vtxStreamIdx < GPU::MAX_VERTEX_STREAMS; ++vtxStreamIdx)
			{
				// Setup stream descs.
				const i32 stride = GPU::GetStride(elements, numElements, vtxStreamIdx);
				if(stride > 0)
				{
					Core::Vector<u8> vertexData(stride * mesh->mNumVertices, 0);
					Core::Vector<Core::StreamDesc> inStreamDescs;
					Core::Vector<Core::StreamDesc> outStreamDescs;
					Core::Vector<i32> numComponents;
					for(i32 elementIdx = 0; elementIdx < numElements; ++elementIdx)
					{
						const auto& element(elements[elementIdx]);
						if(element.streamIdx_ == vtxStreamIdx)
						{
							Core::StreamDesc inStreamDesc;
							if(GetInStreamDesc(inStreamDesc, element.usage_))
							{
								switch(element.usage_)
								{
								case GPU::VertexUsage::POSITION:
									inStreamDesc.data_ = mesh->mVertices;
									break;
								case GPU::VertexUsage::NORMAL:
									inStreamDesc.data_ = mesh->mNormals;
									break;
								case GPU::VertexUsage::TEXCOORD:
									inStreamDesc.data_ = mesh->mTextureCoords[element.usageIdx_];
									break;
								case GPU::VertexUsage::TANGENT:
									inStreamDesc.data_ = mesh->mTangents;
									break;
								case GPU::VertexUsage::BINORMAL:
									inStreamDesc.data_ = mesh->mBitangents;
									break;
								case GPU::VertexUsage::BLENDWEIGHTS:
									inStreamDesc.data_ = blendWeights.data();
									break;
								case GPU::VertexUsage::BLENDINDICES:
									inStreamDesc.data_ = blendIndices.data();
									break;
								case GPU::VertexUsage::COLOR:
									inStreamDesc.data_ = mesh->mColors[element.usageIdx_];
									break;
								default:
									DBG_ASSERT(false);
								}

								DBG_ASSERT(inStreamDesc.data_);

								Core::StreamDesc outStreamDesc;
								if(GetOutStreamDesc(outStreamDesc, element.format_))
								{
									outStreamDesc.data_ = vertexData.data() + element.offset_;

									numComponents.push_back(
									    Core::Min(inStreamDesc.stride_ / (inStreamDesc.numBits_ >> 3),
									        outStreamDesc.stride_ / (outStreamDesc.numBits_ >> 3)));

									outStreamDesc.stride_ = stride;

									inStreamDescs.push_back(inStreamDesc);
									outStreamDescs.push_back(outStreamDesc);
								}
							}
						}
					}

					for(i32 elementStreamIdx = 0; elementStreamIdx < inStreamDescs.size(); ++elementStreamIdx)
					{
						auto inStreamDesc = inStreamDescs[elementStreamIdx];
						auto outStreamDesc = outStreamDescs[elementStreamIdx];

						DBG_ASSERT(vertexData.size() >= (outStreamDesc.stride_ * (i32)mesh->mNumVertices));
						auto retVal = Core::Convert(
						    outStreamDesc, inStreamDesc, mesh->mNumVertices, numComponents[elementStreamIdx]);
						DBG_ASSERT_MSG(retVal, "Unable to convert stream.");
					}

					streams[vtxStreamIdx].Write(vertexData.data(), vertexData.size());
				}
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

		Core::UUID AddTexture(aiMaterial* material, Graphics::ImportMaterial& importMaterial, const char* name,
		    aiTextureType type, i32 idx)
		{
			Core::UUID texUUID;

			aiString aiName;
			aiString path;
			aiTextureMapping textureMapping = aiTextureMapping_UV;
			unsigned int uvIndex = 0;
			float blend = 0.0f;
			aiTextureOp textureOp = aiTextureOp_Multiply;
			aiTextureMapMode textureMapMode = aiTextureMapMode_Wrap;
			if(material->GetTexture(type, idx, &path, &textureMapping, &uvIndex, &blend, &textureOp, &textureMapMode) ==
			    aiReturn_SUCCESS)
			{
				Core::Array<char, Core::MAX_PATH_LENGTH> sourcePath = {0};
				Core::Array<char, Core::MAX_PATH_LENGTH> texturePath = {0};
				Core::Array<char, Core::MAX_PATH_LENGTH> origTexturePath = {0};
				Core::FileSplitPath(sourceFile_.c_str(), sourcePath.data(), sourcePath.size(), nullptr, 0, nullptr, 0);
				Core::FileAppendPath(texturePath.data(), texturePath.size(), sourcePath.data());
				Core::FileAppendPath(texturePath.data(), texturePath.size(), path.C_Str());
				Core::FileNormalizePath(texturePath.data(), texturePath.size(), false);
				context_->GetPathResolver()->OriginalPath(
				    texturePath.data(), origTexturePath.data(), origTexturePath.size());
				importMaterial.textures_[name] = origTexturePath.data();

				// TODO: Setup metadata for textures.
			}

			return texUUID;
		}

		Core::UUID AddMaterial(aiMaterial* material)
		{
			Core::UUID retVal;

			// Grab material name.
			auto materialName = AssimpGetMaterialName(material);

			// Setup material refs if there are matches.
			for(const auto& materialEntry : metaData_.materials_)
			{
				std::regex regex;
				try
				{
					regex = std::regex(materialEntry.regex_.c_str());
				}
				catch(...)
				{
					// TODO: Log Error.
					continue;
				}

				if(std::regex_match(materialName.c_str(), regex))
				{
					auto foundMaterial = addedMaterials_.find(materialName);
					if(foundMaterial == addedMaterials_.end())
					{
						// Find material file name.
						Core::Array<char, Core::MAX_PATH_LENGTH> materialPath = {0};
						Core::Array<char, Core::MAX_PATH_LENGTH> sourceName = {0};
						Core::Array<char, Core::MAX_PATH_LENGTH> sourceExt = {0};
						Core::Array<char, Core::MAX_PATH_LENGTH> origMaterialPath = {0};
						Core::FileSplitPath(sourceFile_.c_str(), materialPath.data(), materialPath.size(),
						    sourceName.data(), sourceName.size(), sourceExt.data(), sourceExt.size());
						strcat_s(materialPath.data(), materialPath.size(), "/materials/");
						Core::FileCreateDir(materialPath.data());

						strcat_s(materialPath.data(), materialPath.size(), sourceName.data());
						strcat_s(materialPath.data(), materialPath.size(), ".");
						strcat_s(materialPath.data(), materialPath.size(), sourceExt.data());
						strcat_s(materialPath.data(), materialPath.size(), ".");
						strcat_s(materialPath.data(), materialPath.size(), materialName.c_str());
						strcat_s(materialPath.data(), materialPath.size(), ".material");

						context_->GetPathResolver()->OriginalPath(
						    materialPath.data(), origMaterialPath.data(), origMaterialPath.size());
						retVal = origMaterialPath.data();

						// If file exists, load and serialize. Otherwise, use template.
						Graphics::ImportMaterial importMaterial;
						if(Core::FileExists(materialPath.data()))
						{
							auto materialFile = Core::File(materialPath.data(), Core::FileFlags::READ);
							auto materialSer = Serialization::Serializer(materialFile, Serialization::Flags::TEXT);
							importMaterial.Serialize(materialSer);
						}
						else
						{
							importMaterial = materialEntry.template_;
						}

						// Attempt to find textures.
						AddTexture(material, importMaterial, "texDiffuse", aiTextureType_DIFFUSE, 0);
						AddTexture(material, importMaterial, "texSpecular", aiTextureType_SPECULAR, 0);
						AddTexture(material, importMaterial, "texMetallic", aiTextureType_AMBIENT, 0);
						AddTexture(material, importMaterial, "texEmissive", aiTextureType_EMISSIVE, 0);
						AddTexture(material, importMaterial, "texHeight", aiTextureType_HEIGHT, 0);
						AddTexture(material, importMaterial, "texNormal", aiTextureType_NORMALS, 0);
						AddTexture(material, importMaterial, "texRoughness", aiTextureType_SHININESS, 0);
						AddTexture(material, importMaterial, "texOpacity", aiTextureType_OPACITY, 0);
						AddTexture(material, importMaterial, "texDisplacement", aiTextureType_DISPLACEMENT, 0);
						AddTexture(material, importMaterial, "texLightmap", aiTextureType_LIGHTMAP, 0);
						AddTexture(material, importMaterial, "texReflection", aiTextureType_REFLECTION, 0);

						{
							Core::FileRemove(materialPath.data());
							auto materialFile =
							    Core::File(materialPath.data(), Core::FileFlags::CREATE | Core::FileFlags::WRITE);
							auto materialSer = Serialization::Serializer(materialFile, Serialization::Flags::TEXT);
							importMaterial.Serialize(materialSer);
						}
						addedMaterials_[materialName] = retVal;
					}
					else
					{
						retVal = foundMaterial->second;
					}
				}
			}

			return retVal;
		}

		Resource::IConverterContext* context_ = nullptr;
		Graphics::MetaDataModel metaData_;
		Core::String sourceFile_;
		const aiScene* scene_ = nullptr;
		Core::Vector<Graphics::NodeDataAoS> nodes_;
		Core::Vector<Graphics::MeshNode> meshNodes_;
		Core::Vector<Graphics::MeshNodeAABB> meshNodeAABBDatas_;
		Core::Vector<Core::Pair<i32, Graphics::MeshNodeBonePalette*>> meshNodeBonePaletteDatas_;
		Core::Vector<Core::Pair<i32, Graphics::MeshNodeInverseBindpose*>> meshNodeInverseBindposeDatas_;
		Core::Vector<MeshData> meshDatas_;

		Core::Map<Core::String, Core::UUID> defaultTextures_;
		Core::Map<Core::String, Core::UUID> addedMaterials_;
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
