#include <glm/gtc/type_ptr.hpp>

#include <spdlog/spdlog.h>

#include <vel/Util/functions.h>
#include <vel/Scene/Stage/Actor/Mesh/AssimpMeshLoader.h>


namespace vel
{
	AssimpMeshLoader::AssimpMeshLoader() :
		impScene(nullptr),
		meshesToLoad(nullptr),
		currentMeshTextureId(0)
	{
		
	}

	void AssimpMeshLoader::reset()
	{
		this->aiImporter.FreeScene();
		this->impScene = nullptr;
		this->preloadData.clear();
		this->meshes.clear();
		this->currentMeshTextureId = 0;
	}

	void AssimpMeshLoader::preProcessNode(aiNode* node)
	{
		if (node != this->impScene->mRootNode && node->mNumMeshes > 0)
		{
			bool hasTexture = false;
			bool hasLightMap = false;
			bool hasBones = false;

			for (unsigned int i = 0; i < node->mNumMeshes; ++i)
			{
				const aiMesh* mesh = this->impScene->mMeshes[node->mMeshes[i]];

				hasTexture |= mesh->HasTextureCoords(0);
				hasLightMap |= mesh->HasTextureCoords(1);
				hasBones |= mesh->HasBones();
			}

			VtxLayout vtxl;
			if (this->headlessMode())
			{
				vtxl = VtxLayout::VTX_POS;
			}
			else
			{
				if (!hasTexture && !hasLightMap && !hasBones)
					vtxl = VtxLayout::VTX_POS_NRML;
				else if (hasTexture && !hasLightMap && !hasBones)
					vtxl = VtxLayout::VTX_POS_NRML_TX;
				else if (hasTexture && hasLightMap && !hasBones)
					vtxl = VtxLayout::VTX_POS_NRML_TX_LM;
				else if (hasTexture && !hasLightMap && hasBones)
					vtxl = VtxLayout::VTX_POS_NRML_TX_SKN;
				else
				{
					SPDLOG_ERROR("AssimpMeshLoader::preProcessNode(): unsupported vertex layout type. Using default");
					vtxl = VtxLayout::VTX_POS;
				}
			}

			this->preloadData.push_back(std::pair<std::string, VtxLayout>(node->mName.C_Str(), vtxl));
		}
			

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			this->preProcessNode(node->mChildren[i]);
	}

	const std::vector<std::pair<std::string, VtxLayout>>& AssimpMeshLoader::preload(const std::string& filePath)
	{
		this->impScene = this->aiImporter.ReadFile(filePath, 
			aiProcess_Triangulate
			| aiProcess_FlipUVs
			| aiProcess_JoinIdenticalVertices
			| aiProcess_LimitBoneWeights // default is 4, which is our max supported
			| aiProcess_GenSmoothNormals // generate smooth normals, when normals not provided
		);

		if (!this->impScene || !this->impScene->mRootNode)
		{
			SPDLOG_DEBUG("AssimpMeshLoader::preload(): {}", this->aiImporter.GetErrorString());
		}
		else
		{
			this->preProcessNode(this->impScene->mRootNode);
		}

		return this->preloadData;
	}

	std::vector<std::unique_ptr<Mesh>> AssimpMeshLoader::load(std::vector<std::pair<std::string, GeoPool*>>* loadables)
	{
		this->meshesToLoad = loadables;

		this->processNode(this->impScene->mRootNode);

		return std::move(this->meshes);
	}

	void AssimpMeshLoader::processVtxPos(aiMesh* aiMesh, Mesh* mesh)
	{
		std::vector<VtxPos>& verts = static_cast<GeoPoolT<VtxPos>*>(mesh->gp)->vertices;

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			VtxPos vtx;
			vtx.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };

			verts.push_back(vtx);
		}
	}

	void AssimpMeshLoader::processVtxPosNrml(aiMesh* aiMesh, Mesh* mesh)
	{
		std::vector<VtxPosNrml>& verts = static_cast<GeoPoolT<VtxPosNrml>*>(mesh->gp)->vertices;

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			VtxPosNrml vtx;
			vtx.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
			vtx.normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };

			verts.push_back(vtx);
		}
	}

	void AssimpMeshLoader::processVtxPosNrmlTx(aiMesh* aiMesh, Mesh* mesh)
	{
		std::vector<VtxPosNrmlTx>& verts = static_cast<GeoPoolT<VtxPosNrmlTx>*>(mesh->gp)->vertices;

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			VtxPosNrmlTx vtx;
			vtx.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
			vtx.normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };

			if (aiMesh->HasTextureCoords(0))
				vtx.textureCoords = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };
			else
				vtx.textureCoords = { 0.f, 0.f };

			vtx.materialUBOIndex = this->currentMeshTextureId;

			verts.push_back(vtx);
		}
	}

	void AssimpMeshLoader::processVtxPosNrmlTxLm(aiMesh* aiMesh, Mesh* mesh)
	{
		std::vector<VtxPosNrmlTxLm>& verts = static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(mesh->gp)->vertices;

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			VtxPosNrmlTxLm vtx;
			vtx.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
			vtx.normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };

			if (aiMesh->HasTextureCoords(0))
				vtx.textureCoords = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };
			else
				vtx.textureCoords = { 0.f, 0.f };

			if (aiMesh->HasTextureCoords(1))
				vtx.lightmapCoords = { aiMesh->mTextureCoords[1][i].x, aiMesh->mTextureCoords[1][i].y };
			else
				vtx.lightmapCoords = { 0.f, 0.f };

			vtx.materialUBOIndex = this->currentMeshTextureId;

			verts.push_back(vtx);
		}
	}

	void AssimpMeshLoader::processVtxPosNrmlTxSkn(aiMesh* aiMesh, Mesh* mesh, unsigned int offset)
	{
		std::vector<VtxPosNrmlTxSkn>& verts = static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(mesh->gp)->vertices;

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			VtxPosNrmlTxSkn vtx;
			vtx.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
			vtx.normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };

			if (aiMesh->HasTextureCoords(0))
				vtx.textureCoords = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };
			else
				vtx.textureCoords = { 0.f, 0.f };

			vtx.materialUBOIndex = this->currentMeshTextureId;

			verts.push_back(vtx);
		}

		for (unsigned int i = 0; i < aiMesh->mNumBones; i++)
		{
			if (aiMesh->mBones[i]->mNumWeights == 0)
				continue;

			std::string aBoneName = aiMesh->mBones[i]->mName.C_Str();
			unsigned int boneIndex;
			bool foundExistingBone = false;
			for (unsigned int k = 0; k < mesh->bones.size(); k++)
			{
				if (mesh->bones[k].name == aBoneName)
				{
					boneIndex = k;
					foundExistingBone = true;
					break;
				}
			}

			if (!foundExistingBone)
			{
				MeshBone b = MeshBone();
				b.name = aiMesh->mBones[i]->mName.C_Str();
				b.offsetMatrix = this->aiMatrix4x4ToGlm(aiMesh->mBones[i]->mOffsetMatrix);
				mesh->bones.push_back(b);
				boneIndex = mesh->bones.size() - 1;
			}

			for (unsigned int j = 0; j < aiMesh->mBones[i]->mNumWeights; j++)
			{
				unsigned int tmpVertInd = mesh->baseVertex + offset + aiMesh->mBones[i]->mWeights[j].mVertexId;
				for (unsigned int m = 0; m < (sizeof(verts[tmpVertInd].boneIds) / sizeof(verts[tmpVertInd].boneIds[0])); m++)
				{
					if (verts[tmpVertInd].boneWeights[m] == 0.0f)
					{
						verts[tmpVertInd].boneIds[m] = boneIndex;
						verts[tmpVertInd].boneWeights[m] = aiMesh->mBones[i]->mWeights[j].mWeight;
						break;
					}
				}
			}
		}
	}

	void AssimpMeshLoader::processMesh(aiMesh* aiMesh, Mesh* mesh)
	{
		unsigned int indiceOffset = mesh->gp->vertexCount() - mesh->baseVertex;

		switch (mesh->gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			this->processVtxPos(aiMesh, mesh);
			break;
		case VtxLayout::VTX_POS_NRML:
			this->processVtxPosNrml(aiMesh, mesh);
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			this->processVtxPosNrmlTx(aiMesh, mesh);
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			this->processVtxPosNrmlTxLm(aiMesh, mesh);
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			this->processVtxPosNrmlTxSkn(aiMesh, mesh, indiceOffset);
			break;
		}

		for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
		{
			aiFace face = aiMesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
				mesh->gp->indices.push_back(indiceOffset + face.mIndices[j]);
		}
	}

	void AssimpMeshLoader::processNode(aiNode* node)
	{
		if (node != this->impScene->mRootNode)
		{
			//
			// Of the meshes to be loaded, find the GeoPool associated with the mesh we are currently loading
			//
			std::string nodeName = node->mName.C_Str();

			GeoPool* gp = nullptr;
			for (auto& m : *this->meshesToLoad)
			{
				if (m.first == nodeName)
				{
					gp = m.second;
					break;
				}
			}

			//
			// If we found a GeoPool (meaning provided mesh name was valid), process data for this mesh
			//
			if (gp)
			{
				// join all aiMeshes of this node into a single Mesh object

				SPDLOG_DEBUG("AssimpMeshLoader::processNode(): Loading new Mesh: {}", nodeName);


				// create one single mesh from all of the aiMeshes
				unsigned int meshCount = node->mNumMeshes;

				// if more than one mesh then we need to sort these meshes by the names of their
				// materials, unless the name of their material is "DefaultMaterial", then we process
				// those after the named material meshes
				std::vector<aiMesh*> defaultMaterialMeshes;
				std::vector<std::pair<std::string, aiMesh*>> customMaterialMeshes;
				for (unsigned int i = 0; i < meshCount; i++)
				{
					aiMesh* tmpMesh = this->impScene->mMeshes[node->mMeshes[(i)]];
					std::string matName = this->impScene->mMaterials[tmpMesh->mMaterialIndex]->GetName().C_Str();
					if (matName == "DefaultMaterial")
					{
						defaultMaterialMeshes.push_back(tmpMesh);
						continue;
					}

					customMaterialMeshes.push_back(std::pair<std::string, aiMesh*>(matName, tmpMesh));
				}

				// now sort meshes based on material names
				std::sort(customMaterialMeshes.begin(), customMaterialMeshes.end());

				// reset mesh texture id... this was a fun little bug to figure out
				this->currentMeshTextureId = 0;

				// init Mesh here. Set it's index values from sizes of GeoPool buffers. Hold tmp count of indices
				std::unique_ptr<Mesh> finalMesh = std::make_unique<Mesh>(nodeName);
				finalMesh->gp = gp;
				finalMesh->firstIndex = gp->indices.size();
				finalMesh->baseVertex = gp->vertexCount();
				unsigned int prevIndexCount = gp->indices.size();

				// process all custom material meshes
				for (auto& cm : customMaterialMeshes)
				{
					this->processMesh(cm.second, finalMesh.get());
					this->currentMeshTextureId++;
				}

				// process all "DefaultMaterial" meshes
				for (auto& dm : defaultMaterialMeshes)
				{
					this->processMesh(dm, finalMesh.get());
					this->currentMeshTextureId++;
				}

				finalMesh->indexCount = finalMesh->gp->indices.size() - prevIndexCount;

				this->meshes.push_back(std::move(finalMesh));
			}
		}

		// Do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			this->processNode(node->mChildren[i]);
	}

	glm::mat4 AssimpMeshLoader::aiMatrix4x4ToGlm(const aiMatrix4x4& from)
	{
		glm::mat4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	}

}