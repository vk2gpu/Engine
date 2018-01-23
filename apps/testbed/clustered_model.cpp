#include "clustered_model.h"
#include "common.h"

#include "core/concurrency.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/half.h"
#include "core/library.h"
#include "core/misc.h"
#include "core/pair.h"
#include "core/string.h"
#include "core/type_conversion.h"
#include "gpu/enum.h"
#include "gpu/manager.h"
#include "gpu/utils.h"
#include "graphics/converters/import_model.h"
#include "job/concurrency.h"
#include "job/function_job.h"
#include "job/manager.h"
#include "math/aabb.h"
#include "math/mat44.h"
#include "resource/converter.h"
#include "serialization/serializer.h"

#include "assimp/config.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

#if ENABLE_SIMPLYGON
#include "SimplygonSDK.h"
#endif

#include <cstring>
#include <regex>
#include <limits>

#if defined(COMPILER_MSVC)
#pragma optimize("", on)
#endif

// Utility code pulled from model converter.
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

#if ENABLE_SIMPLYGON
	SimplygonSDK::ISimplygonSDK* GetSimplygon()
	{
		SimplygonSDK::ISimplygonSDK* sdk = nullptr;
		auto sgLib = Core::LibraryOpen("SimplygonSDKRuntimeReleasex64.dll");
		if(sgLib)
		{
			typedef void (*GetInterfaceVersionSimplygonSDKPtr)(char*);
			GetInterfaceVersionSimplygonSDKPtr GetInterfaceVersionSimplygonSDK =
			    (GetInterfaceVersionSimplygonSDKPtr)Core::LibrarySymbol(sgLib, "GetInterfaceVersionSimplygonSDK");

			typedef int (*InitializeSimplygonSDKPtr)(
			    const char* licenseData, SimplygonSDK::ISimplygonSDK** outInterfacePtr);
			InitializeSimplygonSDKPtr InitializeSimplygonSDK =
			    (InitializeSimplygonSDKPtr)Core::LibrarySymbol(sgLib, "InitializeSimplygonSDK");

			if(GetInterfaceVersionSimplygonSDK == nullptr || InitializeSimplygonSDK == nullptr)
				return nullptr;

			char versionHash[200];
			GetInterfaceVersionSimplygonSDK(versionHash);
			if(strcmp(versionHash, SimplygonSDK::GetInterfaceVersionHash()) != 0)
			{
				DBG_LOG(
				    "Library version mismatch. Header=%s Lib=%s", SimplygonSDK::GetInterfaceVersionHash(), versionHash);
				return nullptr;
			}

			const char* licenseData = nullptr;
			Core::Vector<u8> licenseFile;
			if(auto file = Core::File("../../../../res/simplygon_license.xml", Core::FileFlags::READ))
			{
				licenseFile.resize((i32)file.Size());
				file.Read(licenseFile.data(), file.Size());
				licenseData = (const char*)licenseFile.data();
			}


			i32 result = InitializeSimplygonSDK(licenseData, &sdk);
			if(result != SimplygonSDK::SG_ERROR_NOERROR && result != SimplygonSDK::SG_ERROR_ALREADYINITIALIZED)
			{
				DBG_LOG("Failed to initialize Simplygon. Error: %d.", result);
				sdk = nullptr;
			}

			return sdk;
		}
		return nullptr;
	}
#endif

	Graphics::MaterialRef GetMaterial(Core::String sourceFile, aiMaterial* material)
	{
		Core::UUID retVal;

		// Grab material name.
		auto materialName = AssimpGetMaterialName(material);

		// Find material file name.
		Core::Array<char, Core::MAX_PATH_LENGTH> materialPath = {0};
		Core::Array<char, Core::MAX_PATH_LENGTH> sourceName = {0};
		Core::Array<char, Core::MAX_PATH_LENGTH> sourceExt = {0};
		Core::Array<char, Core::MAX_PATH_LENGTH> origMaterialPath = {0};
		Core::FileSplitPath(sourceFile.c_str(), materialPath.data(), materialPath.size(), sourceName.data(),
		    sourceName.size(), sourceExt.data(), sourceExt.size());
		strcat_s(materialPath.data(), materialPath.size(), "/materials/");
		Core::FileCreateDir(materialPath.data());

		strcat_s(materialPath.data(), materialPath.size(), sourceName.data());
		strcat_s(materialPath.data(), materialPath.size(), ".");
		strcat_s(materialPath.data(), materialPath.size(), sourceExt.data());
		strcat_s(materialPath.data(), materialPath.size(), ".");
		strcat_s(materialPath.data(), materialPath.size(), materialName.c_str());
		strcat_s(materialPath.data(), materialPath.size(), ".material");

		return materialPath.data();
	}

} // namespace

namespace MeshTools
{
	// http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
	// method to seperate bits from a given integer 3 positions apart
	inline uint64_t splitBy3(unsigned int a)
	{
		uint64_t x = a & 0x1fffff; // we only look at the first 21 bits
		x = (x | x << 32) &
		    0x1f00000000ffff; // shift left 32 bits, OR with self, and 00011111000000000000000000000000000000001111111111111111
		x = (x | x << 16) &
		    0x1f0000ff0000ff; // shift left 32 bits, OR with self, and 00011111000000000000000011111111000000000000000011111111
		x = (x | x << 8) &
		    0x100f00f00f00f00f; // shift left 32 bits, OR with self, and 0001000000001111000000001111000000001111000000001111000000000000
		x = (x | x << 4) &
		    0x10c30c30c30c30c3; // shift left 32 bits, OR with self, and 0001000011000011000011000011000011000011000011000011000100000000
		x = (x | x << 2) & 0x1249249249249249;
		return x;
	}

	inline uint64_t MortonEncode(unsigned int x, unsigned int y, unsigned int z)
	{
		uint64_t answer = 0;
		answer |= splitBy3(x) | splitBy3(y) << 1 | splitBy3(z) << 2;
		return answer;
	}

	struct Vertex
	{
		Math::Vec3 position_;
		Math::Vec3 normal_;
		Math::Vec3 tangent_;
		Math::Vec2 texcoord_;
		Math::Vec4 color_;

		u32 hash_ = 0;
		void Initialize()
		{
			hash_ = 0;
			hash_ = Core::HashCRC32(hash_, &position_, sizeof(position_));
			hash_ = Core::HashCRC32(hash_, &normal_, sizeof(normal_));
			hash_ = Core::HashCRC32(hash_, &tangent_, sizeof(tangent_));
			hash_ = Core::HashCRC32(hash_, &texcoord_, sizeof(texcoord_));
			hash_ = Core::HashCRC32(hash_, &color_, sizeof(color_));
		}

		bool operator==(const Vertex& other) const
		{
			if(hash_ != other.hash_)
				return false;
			if(position_ != other.position_)
				return false;
			if(normal_ != other.normal_)
				return false;
			if(tangent_ != other.tangent_)
				return false;
			if(texcoord_ != other.texcoord_)
				return false;
			if(color_ != other.color_)
				return false;
			return true;
		}
	};

	struct Triangle
	{
		Triangle() = default;

		Triangle(i32 a, i32 b, i32 c)
		{
			idx_[0] = a;
			idx_[1] = b;
			idx_[2] = c;
		}

		u64 SortKey(Core::ArrayView<Vertex> vertices, Math::AABB bounds) const
		{
			auto a = vertices[idx_[0]];
			auto b = vertices[idx_[1]];
			auto c = vertices[idx_[2]];

			Math::AABB triBounds;
			triBounds.ExpandBy(a.position_);
			triBounds.ExpandBy(b.position_);
			triBounds.ExpandBy(c.position_);

			Math::Vec3 position = (a.position_ + b.position_ + c.position_) / 3.0f;
			position = (position - bounds.Minimum()) / bounds.Dimensions();
			f32 scaleFactor = 0xff; //0x1fffff; // 21 bits x 3 = 63 bits.
			u32 x = (u32)(position.x * scaleFactor);
			u32 y = (u32)(position.y * scaleFactor);
			u32 z = (u32)(position.z * scaleFactor);
			//u64 s = (u64)((triBounds.Diameter() / bounds.Diameter()) * 4);

			return MortonEncode(x, y, z);
		}

		i32 idx_[3];
	};

	struct Mesh
	{
		Mesh() = default;

		Core::Vector<Vertex> vertices_;
		Core::Vector<u32> vertexHashes_;
		Core::Vector<Triangle> triangles_;
		Math::AABB bounds_;

		void AddFace(Vertex a, Vertex b, Vertex c)
		{
			auto AddVertex = [this](Vertex a) -> i32 {
				i32 idx = vertices_.size();
#if 1
				auto it = std::find_if(vertexHashes_.begin(), vertexHashes_.end(), [&a, this](const u32& b) {
					if(a.hash_ == b)
					{
						i32 idx = (i32)(&b - vertexHashes_.begin());
						return a == vertices_[idx];
					}
					return false;
				});

				if(it != vertexHashes_.end())
					return (i32)(it - vertexHashes_.begin());
#endif
				vertices_.push_back(a);
				vertexHashes_.push_back(a.hash_);
				return idx;
			};

			bounds_.ExpandBy(a.position_);
			bounds_.ExpandBy(b.position_);
			bounds_.ExpandBy(c.position_);
			auto ia = AddVertex(a);
			auto ib = AddVertex(b);
			auto ic = AddVertex(c);
			triangles_.emplace_back(ia, ib, ic);
		}

		void ImportAssimpMesh(const aiMesh* mesh)
		{
			vertices_.reserve(mesh->mNumFaces * 3);
			vertexHashes_.reserve(mesh->mNumFaces * 3);
			triangles_.reserve(mesh->mNumFaces);

			for(unsigned int i = 0; i < mesh->mNumFaces; ++i)
			{
				// Skip anything that isn't a triangle.
				if(mesh->mFaces[i].mNumIndices == 3)
				{
					int ia = mesh->mFaces[i].mIndices[0];
					int ib = mesh->mFaces[i].mIndices[1];
					int ic = mesh->mFaces[i].mIndices[2];

					auto GetVertex = [mesh](int idx) {
						Vertex v = {};
						v.position_ = &mesh->mVertices[idx].x;
						if(mesh->mNormals)
							v.normal_ = &mesh->mNormals[idx].x;
						if(mesh->mTangents)
							v.tangent_ = &mesh->mTangents[idx].x;
						if(mesh->mTextureCoords[0])
							v.texcoord_ = &mesh->mTextureCoords[0][idx].x;
						if(mesh->mColors[0])
							v.color_ = &mesh->mColors[0][idx].r;
						else
							v.color_ = Math::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
						v.Initialize();
						return v;
					};

					Vertex a = GetVertex(ia);
					Vertex b = GetVertex(ib);
					Vertex c = GetVertex(ic);

					AddFace(a, b, c);
				}
			}
		}

		void SortTriangles()
		{
			std::sort(triangles_.begin(), triangles_.end(), [this](const Triangle& a, const Triangle& b) {
				auto aKey = a.SortKey(Core::ArrayView<Vertex>(vertices_.begin(), vertices_.end()), bounds_);
				auto bKey = b.SortKey(Core::ArrayView<Vertex>(vertices_.begin(), vertices_.end()), bounds_);
				return aKey < bKey;
			});
		}

		void ImportMeshCluster(Mesh* mesh, i32 firstTri, i32 numTris)
		{
			if(firstTri >= mesh->triangles_.size())
				DBG_ASSERT(false);

			for(i32 i = firstTri; i < (firstTri + numTris); ++i)
			{
				if(i < mesh->triangles_.size())
				{
					auto tri = mesh->triangles_[i];
					AddFace(mesh->vertices_[tri.idx_[0]], mesh->vertices_[tri.idx_[1]], mesh->vertices_[tri.idx_[2]]);
				}
				else
				{
					// Patch up with degenerates.
					AddFace(vertices_[0], vertices_[0], vertices_[0]);
				}
			}
		}
	};

#if ENABLE_SIMPLYGON
	SimplygonSDK::spGeometryData CreateSGGeometry(SimplygonSDK::ISimplygonSDK* sg, const Mesh* mesh)
	{
		using namespace SimplygonSDK;

		spGeometryData geom = sg->CreateGeometryData();

		geom->SetVertexCount(mesh->vertices_.size());
		geom->SetTriangleCount(mesh->triangles_.size());
		geom->AddMaterialIds();
		geom->AddNormals();
		geom->AddTangents(0);
		geom->AddTexCoords(0);
		geom->AddColors(0);

		auto positions = geom->GetCoords();
		auto normals = geom->GetNormals();
		auto tangents = geom->GetTangents(0);
		auto texcoords = geom->GetTexCoords(0);
		auto colors = geom->GetColors(0);
		auto vertexIds = geom->GetVertexIds();
		auto materialIds = geom->GetMaterialIds();

		DBG_ASSERT(positions->GetTupleSize() == 3);
		DBG_ASSERT(normals->GetTupleSize() == 3);
		DBG_ASSERT(tangents->GetTupleSize() == 3);
		DBG_ASSERT(texcoords->GetTupleSize() == 2);
		DBG_ASSERT(colors->GetTupleSize() == 4);
		DBG_ASSERT(vertexIds->GetTupleSize() == 1);
		DBG_ASSERT(materialIds->GetTupleSize() == 1);

		for(i32 idx = 0; idx < mesh->vertices_.size(); ++idx)
		{
			auto vertex = mesh->vertices_[idx];
			positions->SetTuple(idx, (const f32*)&vertex.position_);
			normals->SetTuple(idx, (const f32*)&vertex.normal_);
			tangents->SetTuple(idx, (const f32*)&vertex.tangent_);
			texcoords->SetTuple(idx, (const f32*)&vertex.texcoord_);
			colors->SetTuple(idx, (const f32*)&vertex.color_);
		}

		for(i32 idx = 0; idx < mesh->triangles_.size(); ++idx)
		{
			vertexIds->SetTuple(idx * 3 + 0, &mesh->triangles_[idx].idx_[0]);
			vertexIds->SetTuple(idx * 3 + 1, &mesh->triangles_[idx].idx_[1]);
			vertexIds->SetTuple(idx * 3 + 2, &mesh->triangles_[idx].idx_[2]);
		}

		for(i32 idx = 0; idx < mesh->triangles_.size(); ++idx)
		{
			materialIds->SetItem(idx, 0);
		}

		return geom;
	}

	SimplygonSDK::spScene CreateSGScene(SimplygonSDK::ISimplygonSDK* sg, Core::ArrayView<Mesh*> meshes)
	{
		using namespace SimplygonSDK;
		spScene scene = sg->CreateScene();

		for(const auto* mesh : meshes)
		{
			spSceneMesh sceneMesh = sg->CreateSceneMesh();

			sceneMesh->SetGeometry(CreateSGGeometry(sg, mesh));

			scene->GetRootNode()->AddChild(sceneMesh);
		}

		return scene;
	}

	Mesh* CreateMesh(SimplygonSDK::ISimplygonSDK* sg, SimplygonSDK::spSceneMesh sceneMesh)
	{
		using namespace SimplygonSDK;

		Mesh* mesh = new Mesh();
		spGeometryData geom = sceneMesh->GetGeometry();

		auto positions = geom->GetCoords();
		auto normals = geom->GetNormals();
		auto tangents = geom->GetTangents(0);
		auto texcoords = geom->GetTexCoords(0);
		auto colors = geom->GetColors(0);
		auto vertexIds = geom->GetVertexIds();
		auto materialIds = geom->GetMaterialIds();

		mesh->vertices_.resize(geom->GetVertexCount());
		mesh->triangles_.resize(geom->GetTriangleCount());

		auto GetVec2 = [](
		    spRealArray arr, i32 idx) { return Math::Vec2(arr->GetItem(idx * 2), arr->GetItem(idx * 2 + 1)); };

		auto GetVec3 = [](spRealArray arr, i32 idx) {
			return Math::Vec3(arr->GetItem(idx * 3), arr->GetItem(idx * 3 + 1), arr->GetItem(idx * 3 + 2));
		};

		auto GetVec4 = [](spRealArray arr, i32 idx) {
			return Math::Vec4(
			    arr->GetItem(idx * 4), arr->GetItem(idx * 4 + 1), arr->GetItem(idx * 4 + 2), arr->GetItem(idx * 4 + 3));
		};

		for(i32 idx = 0; idx < mesh->vertices_.size(); ++idx)
		{
			auto& vertex = mesh->vertices_[idx];

			vertex.position_ = GetVec3(positions, idx);
			vertex.normal_ = GetVec3(normals, idx);
			vertex.tangent_ = GetVec3(tangents, idx);
			vertex.texcoord_ = GetVec2(texcoords, idx);
			vertex.color_ = GetVec4(colors, idx);

			mesh->bounds_.ExpandBy(vertex.position_);
		}

		for(i32 idx = 0; idx < mesh->vertices_.size(); ++idx)
		{
			auto& vertex = mesh->vertices_[idx];
			vertex.Initialize();
		}

		for(i32 idx = 0; idx < mesh->triangles_.size(); ++idx)
		{
			auto& triangle = mesh->triangles_[idx];

			triangle.idx_[0] = vertexIds->GetItem(idx * 3 + 0);
			triangle.idx_[1] = vertexIds->GetItem(idx * 3 + 1);
			triangle.idx_[2] = vertexIds->GetItem(idx * 3 + 2);
		}

		return mesh;
	}

	Mesh* ReduceMesh(SimplygonSDK::ISimplygonSDK* sg, Mesh* mesh, f32 ratio)
	{
		auto sgScene = MeshTools::CreateSGScene(sg, mesh);
		auto rp = sg->CreateReductionProcessor();
		auto settings = rp->GetReductionSettings();

		settings->SetTriangleRatio(ratio);
		rp->SetScene(sgScene);

		rp->RunProcessing();

		for(i32 idx = 0; idx < (i32)sgScene->GetRootNode()->GetChildCount(); ++idx)
		{
			auto childNode = sgScene->GetRootNode()->GetChild(idx);
			if(auto* meshNode = SimplygonSDK::ISceneMesh::SafeCast(childNode))
			{
				return MeshTools::CreateMesh(sg, meshNode);
			}
		}
		return nullptr;
	}
#endif

} // namespace MeshTools

ClusteredModel::ClusteredModel(const char* sourceFile)
{
	const aiScene* scene = nullptr;

	Core::String fileName = "../../../../res/";
	fileName.Append(sourceFile);

	auto propertyStore = aiCreatePropertyStore();
	aiLogStream assimpLogger = {AssimpLogStream, (char*)this};
	{
		Core::ScopedMutex lock(assimpMutex_);
		aiAttachLogStream(&assimpLogger);

		int flags = aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_FindDegenerates | aiProcess_SortByPType |
		            aiProcess_FindInvalidData | aiProcess_RemoveRedundantMaterials | aiProcess_SplitLargeMeshes |
		            aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure | aiProcess_SplitByBoneCount |
		            aiProcess_LimitBoneWeights | aiProcess_MakeLeftHanded | aiProcess_FlipUVs |
		            aiProcess_FlipWindingOrder | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes |
		            aiProcess_RemoveComponent;
		aiSetImportPropertyInteger(
		    propertyStore, AI_CONFIG_PP_RVC_FLAGS, aiComponent_ANIMATIONS | aiComponent_LIGHTS | aiComponent_CAMERAS);
		aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 256 * 1024);

		scene = aiImportFileExWithProperties(fileName.c_str(), flags, nullptr, propertyStore);

		aiReleasePropertyStore(propertyStore);
		aiDetachLogStream(&assimpLogger);
	}

	if(scene)
	{
		i32 clusterSize = 64;
		Core::Vector<MeshTools::Mesh*> meshes;
		Core::Vector<MeshTools::Mesh*> meshClusters;
		i32 numVertices = 0;
		i32 numIndices = 0;

		// Create meshes.
		for(unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			auto* mesh = new MeshTools::Mesh();
			meshes.push_back(mesh);
		}

		// Spin up jobs for all meshes to perform importing.
		Job::FunctionJob importJob("cluster_model_import",
		    [&scene, &meshes](i32 param) { meshes[param]->ImportAssimpMesh(scene->mMeshes[param]); });
		Job::Counter* counter = nullptr;
		importJob.RunMultiple(0, meshes.size() - 1, &counter);
		Job::Manager::WaitForCounter(counter, 0);

		Job::FunctionJob sortJob("cluster_model_sort", [&meshes](i32 param) { meshes[param]->SortTriangles(); });
		sortJob.RunMultiple(0, meshes.size() - 1, &counter);
		Job::Manager::WaitForCounter(counter, 0);

		for(i32 i = 0; i < meshes.size(); ++i)
		{
			auto* mesh = meshes[i];
			auto material = GetMaterial(sourceFile, scene->mMaterials[scene->mMeshes[i]->mMaterialIndex]);
			if(!material)
				material = "default.material";
			materials_.emplace_back(std::move(material));

			Core::Log("Mesh %u: Diameter: %.3f\n", i, mesh->bounds_.Diameter());

			Mesh outMesh;
			outMesh.baseCluster_ = clusters_.size();
			outMesh.numClusters_ = (mesh->triangles_.size() + clusterSize - 1) / clusterSize;
			meshes_.push_back(outMesh);

			for(i32 triIdx = 0; triIdx < mesh->triangles_.size(); triIdx += clusterSize)
			{
				auto* meshCluster = new MeshTools::Mesh();
				meshCluster->ImportMeshCluster(mesh, triIdx, clusterSize);
				meshClusters.push_back(meshCluster);

				MeshCluster outMeshCluster;
				outMeshCluster.meshIdx_ = i;
				outMeshCluster.baseDrawArg_ = outMesh.baseCluster_;
				outMeshCluster.baseVertex_ = numVertices;
				outMeshCluster.baseIndex_ = numIndices;
				outMeshCluster.numIndices_ = meshCluster->triangles_.size() * 3;
				clusters_.push_back(outMeshCluster);
				clusterBounds_.push_back(meshCluster->bounds_);

				numVertices += meshCluster->vertices_.size();
				numIndices += meshCluster->triangles_.size() * 3;
			}
		}

		// Setup vertex declaration.
		Core::Array<GPU::VertexElement, GPU::MAX_VERTEX_ELEMENTS> elements;
		i32 numElements = 0;
		i32 currStream = 0;

		// Vertex format.
		elements[numElements++] =
		    GPU::VertexElement(currStream, 0, GPU::Format::R32G32B32_FLOAT, GPU::VertexUsage::POSITION, 0);
		currStream++;

		elements[numElements++] =
		    GPU::VertexElement(currStream, 0, GPU::Format::R8G8B8A8_SNORM, GPU::VertexUsage::NORMAL, 0);

		elements[numElements++] =
		    GPU::VertexElement(currStream, 0, GPU::Format::R16G16_FLOAT, GPU::VertexUsage::TEXCOORD, 0);
		currStream++;

		elements[numElements++] =
		    GPU::VertexElement(currStream, 0, GPU::Format::R8G8B8A8_UNORM, GPU::VertexUsage::COLOR, 0);
		currStream++;

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

		elements_.insert(elements.begin(), elements.begin() + numElements);

		Core::Array<BinaryStream, GPU::MAX_VERTEX_STREAMS> streams;
		BinaryStream idxStream;

		i32 indexOffset = 0;
		auto cluster = clusters_.begin();
		for(const auto& meshCluster : meshClusters)
		{
			indexOffset = cluster->baseVertex_;
			++cluster;

			for(i32 triIdx = 0; triIdx < meshCluster->triangles_.size(); ++triIdx)
			{
				auto tri = meshCluster->triangles_[triIdx];
				idxStream.Write(tri.idx_[0] + indexOffset);
				idxStream.Write(tri.idx_[1] + indexOffset);
				idxStream.Write(tri.idx_[2] + indexOffset);
			}

			for(i32 vtxStreamIdx = 0; vtxStreamIdx < GPU::MAX_VERTEX_STREAMS; ++vtxStreamIdx)
			{
				// Setup stream descs.
				const i32 stride = GPU::GetStride(elements.data(), numElements, vtxStreamIdx);
				if(stride > 0)
				{
					Core::Vector<u8> vertexData(stride * meshCluster->vertices_.size(), 0);
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
								inStreamDesc.stride_ = sizeof(MeshTools::Vertex);
								switch(element.usage_)
								{
								case GPU::VertexUsage::POSITION:
									inStreamDesc.data_ = &meshCluster->vertices_.data()->position_;
									break;
								case GPU::VertexUsage::NORMAL:
									inStreamDesc.data_ = &meshCluster->vertices_.data()->normal_;
									break;
								case GPU::VertexUsage::TEXCOORD:
									inStreamDesc.data_ = &meshCluster->vertices_.data()->texcoord_;
									break;
								case GPU::VertexUsage::TANGENT:
									inStreamDesc.data_ = &meshCluster->vertices_.data()->tangent_;
									break;
								case GPU::VertexUsage::COLOR:
									inStreamDesc.data_ = &meshCluster->vertices_.data()->color_;
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

						DBG_ASSERT(vertexData.size() >= (outStreamDesc.stride_ * (i32)meshCluster->vertices_.size()));
						auto retVal = Core::Convert(outStreamDesc, inStreamDesc, meshCluster->vertices_.size(),
						    numComponents[elementStreamIdx]);
						DBG_ASSERT_MSG(retVal, "Unable to convert stream.");
					}

					streams[vtxStreamIdx].Write(vertexData.data(), vertexData.size());
				}
			}
		}

		BinaryStream vtxStream;

		// Create buffers.
		vertexDesc_.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		vertexDesc_.size_ = 0;
		for(i32 i = 0; i < currStream; ++i)
		{
			vertexDesc_.size_ += streams[i].Size();
			vtxStream.Write(streams[i].Data(), streams[i].Size());
		}

		vertexBuffer_ = GPU::Manager::CreateBuffer(vertexDesc_, vtxStream.Data(), "clustered_model_vb");

		indexDesc_.bindFlags_ = GPU::BindFlags::INDEX_BUFFER | GPU::BindFlags::SHADER_RESOURCE;
		indexDesc_.size_ = numIndices * 4;
		indexBuffer_ = GPU::Manager::CreateBuffer(indexDesc_, idxStream.Data(), "clustered_model_ib");

		boundsDesc_.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		boundsDesc_.size_ = clusterBounds_.size() * sizeof(Math::AABB);
		boundsBuffer_ = GPU::Manager::CreateBuffer(boundsDesc_, clusterBounds_.data(), "clustered_model_bounds");

		clusterDesc_.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		clusterDesc_.size_ = clusters_.size() * sizeof(MeshCluster);
		clusterBuffer_ = GPU::Manager::CreateBuffer(clusterDesc_, clusters_.data(), "clustered_model_clusters");

		drawArgsDesc_.bindFlags_ =
		    GPU::BindFlags::INDEX_BUFFER | GPU::BindFlags::UNORDERED_ACCESS | GPU::BindFlags::INDIRECT_BUFFER;
		drawArgsDesc_.size_ = clusters_.size() * sizeof(GPU::DrawIndexedArgs);
		drawArgsBuffer_ = GPU::Manager::CreateBuffer(drawArgsDesc_, nullptr, "clustered_model_draw_args");

		drawCountDesc_.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS | GPU::BindFlags::INDIRECT_BUFFER;
		drawCountDesc_.size_ = meshes_.size() * sizeof(GPU::DispatchArgs);
		drawCountBuffer_ = GPU::Manager::CreateBuffer(drawCountDesc_, nullptr, "clustered_model_draw_count");

		GPU::DrawBindingSetDesc dbsDesc;
		i32 offset = 0;
		for(i32 streamIdx = 0; streamIdx < currStream; ++streamIdx)
		{
			const i32 stride = GPU::GetStride(elements.data(), numElements, streamIdx);
			dbsDesc.vbs_[streamIdx].resource_ = vertexBuffer_;
			dbsDesc.vbs_[streamIdx].offset_ = offset;
			dbsDesc.vbs_[streamIdx].size_ = stride * numVertices;
			dbsDesc.vbs_[streamIdx].stride_ = stride;

			offset += stride * numVertices;
		}

		dbsDesc.ib_.resource_ = indexBuffer_;
		dbsDesc.ib_.offset_ = 0;
		dbsDesc.ib_.size_ = (i32)indexDesc_.size_;
		dbsDesc.ib_.stride_ = 4;
		dbs_ = GPU::Manager::CreateDrawBindingSet(dbsDesc, "clustered_model_dbs");

		coreShader_ = "shaders/clustered_model.esf";
		coreShader_.WaitUntilReady();

		techs_.resize(materials_.size());
		for(i32 i = 0; i < materials_.size(); ++i)
		{
			materials_[i].WaitUntilReady();

			techs_[i].material_ = materials_[i];
		}

		cullClusterTech_ = coreShader_->CreateTechnique("TECH_CULL_CLUSTERS", Graphics::ShaderTechniqueDesc());
		DBG_ASSERT(cullClusterTech_);

		techDesc_.SetVertexElements(elements_);
		techDesc_.SetTopology(GPU::TopologyType::TRIANGLE);
	}
}

ClusteredModel::~ClusteredModel()
{
	GPU::Manager::DestroyResource(vertexBuffer_);
	GPU::Manager::DestroyResource(indexBuffer_);
	GPU::Manager::DestroyResource(boundsBuffer_);
	GPU::Manager::DestroyResource(clusterBuffer_);
	GPU::Manager::DestroyResource(drawArgsBuffer_);
	GPU::Manager::DestroyResource(drawCountBuffer_);
	GPU::Manager::DestroyResource(dbs_);
}

void ClusteredModel::DrawClusters(DrawContext& drawCtx, ObjectConstants object)
{
	if(auto event = drawCtx.cmdList_.Eventf(0x0, "ClusteredModel"))
	{
		i32 numObjects = 1;
		const i32 objectDataSize = sizeof(ObjectConstants);


		// Allocate command list memory.
		auto* objects = drawCtx.cmdList_.Alloc<ObjectConstants>(numObjects);

		// Update all render packet uniforms.
		for(i32 idx = 0; idx < numObjects; ++idx)
			objects[idx] = object;
		drawCtx.cmdList_.UpdateBuffer(drawCtx.objectSBHandle_, 0, sizeof(ObjectConstants) * numObjects, objects);

		if(enableCulling_)
		{
			static Job::SpinLock spinLock;
			Job::ScopedSpinLock lock(spinLock);

			if(!cullClusterBindings_)
			{
				cullClusterBindings_ = coreShader_->CreateBindingSet("ClusterBindings");
			}

			if(!objectBindings_)
			{
				objectBindings_ = coreShader_->CreateBindingSet("ObjectBindings");
			}

			cullClusterBindings_.Set("inCluster",
			    GPU::Binding::Buffer(clusterBuffer_, GPU::Format::INVALID, 0, clusters_.size(), sizeof(MeshCluster)));
			cullClusterBindings_.Set("inClusterBounds",
			    GPU::Binding::Buffer(
			        boundsBuffer_, GPU::Format::INVALID, 0, clusterBounds_.size(), sizeof(Math::AABB)));
			cullClusterBindings_.Set("outDrawArgs",
			    GPU::Binding::RWBuffer(
			        drawArgsBuffer_, GPU::Format::INVALID, 0, clusters_.size(), sizeof(GPU::DrawIndexedArgs)));
			cullClusterBindings_.Set("outDrawCount",
			    GPU::Binding::RWBuffer(drawCountBuffer_, GPU::Format::INVALID, 0, meshes_.size(), sizeof(u32)));

			objectBindings_.Set(
			    "inObject", GPU::Binding::Buffer(drawCtx.objectSBHandle_, GPU::Format::INVALID, 0, 1, objectDataSize));

			if(auto cullClusterBind = drawCtx.shaderCtx_.BeginBindingScope(cullClusterBindings_))
			{
				if(auto objectBind = drawCtx.shaderCtx_.BeginBindingScope(objectBindings_))
				{
					const i32 groupSize = 64;

					GPU::Handle ps;
					Core::ArrayView<GPU::PipelineBinding> pb;
					if(drawCtx.shaderCtx_.CommitBindings(cullClusterTech_, ps, pb))
					{
						Core::Vector<u32> drawCountData(meshes_.size(), 0);
						drawCtx.cmdList_.UpdateBuffer(drawCountBuffer_, 0, drawCountData.size() * sizeof(u32),
						    drawCtx.cmdList_.Push(drawCountData.data(), drawCountData.size()));

						drawCtx.cmdList_.Dispatch(ps, pb, (clusters_.size() + (groupSize - 1)) / groupSize, 1, 1);
					}
				}
			}
		}

		// Perform draws.
		if(enableCulling_)
		{
			for(i32 idx = 0; idx < numObjects; ++idx)
			{
				for(i32 meshIdx = 0; meshIdx < meshes_.size(); ++meshIdx)
				{
					auto it = techs_[meshIdx].passIndices_.find(drawCtx.passName_);
					if(it != techs_[meshIdx].passIndices_.end())
					{
						const auto& mesh = meshes_[meshIdx];
						if(mesh.numClusters_ > 0)
						{
							auto& tech = techs_[meshIdx].passTechniques_[it->second];
							if(drawCtx.customBindFn_)
								drawCtx.customBindFn_(techs_[meshIdx].material_->GetShader(), tech);

							objectBindings_.Set("inObject",
							    GPU::Binding::Buffer(
							        drawCtx.objectSBHandle_, GPU::Format::INVALID, 0, 1, objectDataSize));

							if(auto objectBind = drawCtx.shaderCtx_.BeginBindingScope(objectBindings_))
							{
								GPU::Handle ps;
								Core::ArrayView<GPU::PipelineBinding> pb;
								if(drawCtx.shaderCtx_.CommitBindings(tech, ps, pb))
								{
									drawCtx.cmdList_.DrawIndirect(ps, pb, dbs_, drawCtx.fbs_, drawCtx.drawState_,
									    GPU::PrimitiveTopology::TRIANGLE_LIST, drawArgsBuffer_,
									    mesh.baseCluster_ * sizeof(GPU::DrawIndexedArgs), drawCountBuffer_,
									    meshIdx * sizeof(i32), mesh.numClusters_);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			for(i32 idx = 0; idx < numObjects; ++idx)
			{
				for(i32 meshIdx = 0; meshIdx < meshes_.size(); ++meshIdx)
				{
					auto it = techs_[meshIdx].passIndices_.find(drawCtx.passName_);
					if(it != techs_[meshIdx].passIndices_.end())
					{
						const auto& mesh = meshes_[meshIdx];
						if(mesh.numClusters_ > 0)
						{
							auto& tech = techs_[meshIdx].passTechniques_[it->second];
							if(drawCtx.customBindFn_)
								drawCtx.customBindFn_(techs_[meshIdx].material_->GetShader(), tech);

							objectBindings_.Set("inObject",
							    GPU::Binding::Buffer(
							        drawCtx.objectSBHandle_, GPU::Format::INVALID, 0, 1, objectDataSize));
							if(auto objectBind = drawCtx.shaderCtx_.BeginBindingScope(objectBindings_))
							{
								GPU::Handle ps;
								Core::ArrayView<GPU::PipelineBinding> pb;
								if(drawCtx.shaderCtx_.CommitBindings(tech, ps, pb))
								{
									auto baseCluster = clusters_[meshes_[meshIdx].baseCluster_];
									drawCtx.cmdList_.Draw(ps, pb, dbs_, drawCtx.fbs_, drawCtx.drawState_,
									    GPU::PrimitiveTopology::TRIANGLE_LIST, baseCluster.baseIndex_, 0,
									    mesh.numClusters_ * baseCluster.numIndices_, 0, 1);
								}
							}
						}
					}
				}
			}
		}
	}
}
