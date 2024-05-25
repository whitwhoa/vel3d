#include <iostream>

#include "vel/functions.h"
#include "vel/Log.h"

#include "vel/AssimpMeshLoader.h"

namespace vel
{
	AssimpMeshLoader::AssimpMeshLoader() :
		impScene(nullptr),
		currentAssetFile(""),
		armatureInFile(""),
		armatureToLoad(""),
		armature(nullptr),
		currentMeshTextureId(0),
		currentGlobalInverseMatrix(glm::mat4())
	{
		
	}

	void AssimpMeshLoader::reset()
	{
		this->aiImporter.FreeScene();
		this->impScene = nullptr;
		this->currentAssetFile = "";
		this->meshesInFile.clear();
		this->armatureInFile = "";
		this->meshesToLoad.clear();
		this->armatureToLoad = "";
		this->meshes.clear();
		this->armature = nullptr;
		this->currentMeshTextureId = 0;
		this->processedNodes.clear();
		this->currentMeshBones.clear();
		this->currentGlobalInverseMatrix = glm::mat4();
	}

	std::pair<std::vector<std::string>, std::string> AssimpMeshLoader::preload(const std::string& filePath)
	{
		this->currentAssetFile = filePath;
		std::pair<std::vector<std::string>, std::string> output;

		this->impScene = this->aiImporter.ReadFile(this->currentAssetFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

		if (!this->impScene || !this->impScene->mRootNode)
		{
			std::string errorMessage = this->aiImporter.GetErrorString();
			std::cout << "ERROR::ASSIMP::" << errorMessage << "\n";
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		this->preProcessNode(this->impScene->mRootNode);

		this->processedNodes.clear();

		output.first = this->meshesInFile;
		output.second = this->armatureInFile;

		return output;
	}

	std::pair<std::vector<std::unique_ptr<Mesh>>, std::unique_ptr<Armature>>
		AssimpMeshLoader::load(const std::pair<const std::vector<std::string>, const std::string>& loadables)
	{
		this->meshesToLoad = loadables.first;
		this->armatureToLoad = loadables.second;

		this->processNode(this->impScene->mRootNode);

		return std::pair<std::vector<std::unique_ptr<Mesh>>, std::unique_ptr<Armature>>(std::move(this->meshes), std::move(this->armature));
	}

	bool AssimpMeshLoader::isRootArmatureNode(aiNode* node)
	{
		// if this node has children, and no mesh, assume that it is an armature
		// (this is very naive, but should work for the time being so we can move forward
		// with skeletal animation implementation. This method can be revised in the future
		// to be more accurate)
		std::string nodeName = node->mParent->mName.C_Str();
		if (nodeName == "RootNode" && node->mNumChildren > 0 && node->mNumMeshes == 0)
			return true;
		else
			return false;
	}

	bool AssimpMeshLoader::nodeHasBeenProcessed(aiNode* in)
	{
		for (auto& pn : this->processedNodes)
			if (pn == in)
				return true;

		return false;
	}

	void AssimpMeshLoader::preProcessNode(aiNode* node)
	{
		std::string nodeName = node->mName.C_Str();
		//std::cout << "processNode:" << nodeName << std::endl;

		// If this is not the RootNode and this node has not already been processed
		if (nodeName != "RootNode" && !this->nodeHasBeenProcessed(node))
		{
			if (this->isRootArmatureNode(node))
				this->armatureInFile = nodeName;
			else
				this->meshesInFile.push_back(nodeName);
		}

		// Do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			this->preProcessNode(node->mChildren[i]);
	}

	void AssimpMeshLoader::processArmatureNode(aiNode* node)
	{
		std::string nodeName = node->mName.C_Str();

		if (nodeName != "RootNode" && !string_contains("_end", nodeName))
		{

			std::string boneName = nodeName;
			std::string nodeParentName = node->mParent->mName.C_Str();

			if (nodeParentName == "RootNode")
			{
#ifdef DEBUG_LOG
				Log::toCliAndFile("Loading new Armature: " + nodeName);
#endif	
				this->armature = std::make_unique<Armature>(boneName);
			}

			ArmatureBone bone;
			bone.name = boneName;
			bone.parentName = nodeParentName == "RootNode" ? boneName : node->mParent->mName.C_Str();
			bone.parentArmature = this->armature.get();

			this->armature->addBone(bone);
		}

		this->processedNodes.push_back(node);

		// Do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			this->processArmatureNode(node->mChildren[i]);
	}

	void AssimpMeshLoader::processAnimations()
	{
		for (unsigned int i = 0; i < this->impScene->mNumAnimations; i++)
		{
			// create a new animation
			std::shared_ptr<Animation> a = std::make_shared<Animation>();
			std::string animationName = this->impScene->mAnimations[i]->mName.C_Str();
			a->name = explode_string(animationName, '|')[1];
			a->duration = this->impScene->mAnimations[i]->mDuration;
			a->tps = this->impScene->mAnimations[i]->mTicksPerSecond;

			// add all channels to animation
			for (unsigned int j = 0; j < this->impScene->mAnimations[i]->mNumChannels; j++)
			{
				auto c = Channel();

				// positions
				for (unsigned int k = 0; k < this->impScene->mAnimations[i]->mChannels[j]->mNumPositionKeys; k++)
				{
					auto position = this->impScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue;
					auto time = (float)this->impScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
					c.positionKeyTimes.push_back(time);
					c.positionKeyValues.push_back(glm::vec3(position.x, position.y, position.z));
				}

				// rotations
				for (unsigned int k = 0; k < this->impScene->mAnimations[i]->mChannels[j]->mNumRotationKeys; k++)
				{
					auto rotation = this->impScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue;
					auto time = (float)this->impScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
					c.rotationKeyTimes.push_back(time);
					c.rotationKeyValues.push_back(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));
				}

				// scalings
				for (unsigned int k = 0; k < this->impScene->mAnimations[i]->mChannels[j]->mNumScalingKeys; k++)
				{
					auto scale = this->impScene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue;
					auto time = (float)this->impScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
					c.scalingKeyTimes.push_back(time);
					c.scalingKeyValues.push_back(glm::vec3(scale.x, scale.y, scale.z));
				}

				// add channel to animation
				a->channels[this->impScene->mAnimations[i]->mChannels[j]->mNodeName.C_Str()] = c;
			}

			// add this animation name/index to the armature's animations vector
			this->armature->addAnimation(a);
		}
	}

	void AssimpMeshLoader::processMesh(aiMesh* aiMesh, std::vector<Vertex>& meshVertices, 
		std::vector<unsigned int>& meshIndices, std::vector<MeshBone>& meshBones)
	{
		unsigned int indiceOffset = meshVertices.size();

		// walk through each of the aimesh's vertices
		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;

			// position
			vector.x = aiMesh->mVertices[i].x;
			vector.y = aiMesh->mVertices[i].y;
			vector.z = aiMesh->mVertices[i].z;
			vertex.position = vector;

			// normal
			vector.x = aiMesh->mNormals[i].x;
			vector.y = aiMesh->mNormals[i].y;
			vector.z = aiMesh->mNormals[i].z;
			vertex.normal = vector;

			// texture coordinates
			if (aiMesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have more than 2 texture coordinates.
				vec.x = aiMesh->mTextureCoords[0][i].x;
				vec.y = aiMesh->mTextureCoords[0][i].y;
				vertex.textureCoordinates = vec;

				// assign this mesh's textureid to each vertex
				vertex.materialUBOIndex = this->currentMeshTextureId;
			}
			else
			{
				vertex.textureCoordinates = glm::vec2(0.0f, 0.0f);
			}

			// secondary texture coordinates
			if (aiMesh->mTextureCoords[1])
			{
				glm::vec2 vec;
				vec.x = aiMesh->mTextureCoords[1][i].x;
				vec.y = aiMesh->mTextureCoords[1][i].y;
				vertex.lightmapCoordinates = vec;
			}
			else
			{
				vertex.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
			}

			meshVertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
		{
			aiFace face = aiMesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
				meshIndices.push_back(face.mIndices[j] + indiceOffset);
		}

		if (aiMesh->HasBones())
		{
			for (unsigned int i = 0; i < aiMesh->mNumBones; i++)
			{
				if (aiMesh->mBones[i]->mNumWeights == 0)
					continue;

				std::string aBoneName = aiMesh->mBones[i]->mName.C_Str();
				unsigned int boneIndex;
				bool foundExistingBone = false;
				for (unsigned int k = 0; k < meshBones.size(); k++)
				{
					if (meshBones.at(k).name == aBoneName)
					{
						boneIndex = k;
						foundExistingBone = true;
						break;
					}
				}

				if (!foundExistingBone)
				{
					auto b = MeshBone();
					b.name = aiMesh->mBones[i]->mName.C_Str();
					b.offsetMatrix = this->aiMatrix4x4ToGlm(aiMesh->mBones[i]->mOffsetMatrix);
					meshBones.push_back(b);
					boneIndex = meshBones.size() - 1;
				}

				for (unsigned int j = 0; j < aiMesh->mBones[i]->mNumWeights; j++)
				{
					unsigned int tmpVertInd = aiMesh->mBones[i]->mWeights[j].mVertexId + indiceOffset;
					for (unsigned int m = 0; m < (sizeof(meshVertices.at(tmpVertInd).weights.ids) / sizeof(meshVertices.at(tmpVertInd).weights.ids[0])); m++)
					{
						if (meshVertices.at(tmpVertInd).weights.weights[m] == 0.0f)
						{
							meshVertices.at(tmpVertInd).weights.ids[m] = boneIndex;
							meshVertices.at(tmpVertInd).weights.weights[m] = aiMesh->mBones[i]->mWeights[j].mWeight;
							break;
						}
					}
				}
			}
		}
	}

	void AssimpMeshLoader::processNode(aiNode* node)
	{
		std::string nodeName = node->mName.C_Str();
		//std::cout << "processNode:" << nodeName << std::endl;


		if (nodeName == "RootNode")
			this->currentGlobalInverseMatrix = glm::inverse(this->aiMatrix4x4ToGlm(node->mTransformation));


		// If this is not the RootNode and this node has not already been processed
		if (nodeName != "RootNode" && !this->nodeHasBeenProcessed(node))
		{
			if (nodeName == this->armatureToLoad)
			{
				this->processArmatureNode(node);

				this->processAnimations();

				// Obtain parent indexes for each bone using their
				// boneNames (done so that these indexes can be used at runtime instead
				// of loops and string comparisons)
				for (auto& b : this->armature->getBones())
					b.parent = this->armature->getBoneIndex(b.parentName);
			}
			else
			{
				// use node name for mesh name

				bool shouldLoadMesh = false;
				for (auto& mn : this->meshesToLoad)
				{
					if (mn == nodeName)
					{
						shouldLoadMesh = true;
						break;
					}
				}

				if (!shouldLoadMesh)
					return;


				// otherwise loop through all meshes for this node, creating a Mesh for each one
#ifdef DEBUG_LOG
				Log::toCliAndFile("Loading new Mesh: " + nodeName);
#endif

				// create one single mesh from all of the aiMeshes

				std::vector<Vertex> meshVertices = {};
				std::vector<unsigned int> meshIndices = {};
				std::vector<MeshBone> meshBones = {};

				auto meshCount = node->mNumMeshes;

				// if more than one mesh then we need to sort these meshes by the names of their
				// materials, unless the name of their material is "DefaultMaterial", then we process
				// those after the named material meshes
				std::vector<aiMesh*> defaultMaterialMeshes;
				std::vector<std::pair<std::string, aiMesh*>> customMaterialMeshes;
				for (unsigned int i = 0; i < meshCount; i++)
				{
					auto tmpMesh = this->impScene->mMeshes[node->mMeshes[(i)]];
					auto matName = this->impScene->mMaterials[tmpMesh->mMaterialIndex]->GetName().C_Str();
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

				// process all custom material meshes
				for (auto& cm : customMaterialMeshes)
				{
					this->processMesh(cm.second, meshVertices, meshIndices, meshBones);
					this->currentMeshTextureId++;
				}

				// process all "DefaultMaterial" meshes
				for (auto& dm : defaultMaterialMeshes)
				{
					this->processMesh(dm, meshVertices, meshIndices, meshBones);
					this->currentMeshTextureId++;
				}

				std::unique_ptr<Mesh> finalMesh = std::make_unique<Mesh>(nodeName);
				finalMesh->setVertices(meshVertices);
				finalMesh->setIndices(meshIndices);
				finalMesh->setBones(meshBones);
				finalMesh->setGlobalInverseMatrix(this->currentGlobalInverseMatrix);

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

	aiMatrix4x4 AssimpMeshLoader::glmToAssImpMat4(glm::mat4 mat)
	{
		const float* glmMat = (const float*)glm::value_ptr(mat);

		return aiMatrix4x4(
			glmMat[0], glmMat[1], glmMat[2], glmMat[3],
			glmMat[4], glmMat[5], glmMat[6], glmMat[7],
			glmMat[8], glmMat[9], glmMat[10], glmMat[11],
			glmMat[12], glmMat[13], glmMat[14], glmMat[15]
		);
	}

}