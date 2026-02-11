#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>


#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "vel/Mesh.h"
#include "vel/VertexBoneData.h"
#include "vel/MeshLoaderInterface.h"


namespace vel
{
	class AssimpMeshLoader : public MeshLoaderInterface
	{
	private:
		Assimp::Importer						aiImporter;
		const aiScene*							impScene;
		std::string								currentAssetFile;
		std::vector<std::string>				meshesInFile;
		std::string								armatureInFile;
		const std::vector<std::string>*			meshesToLoad;
		std::vector<std::unique_ptr<Mesh>>		meshes;
		unsigned int							currentMeshTextureId;

		void			preProcessNode(aiNode* node);

		void			processNode(aiNode* node);
		void			processMesh(aiMesh* aiMesh, std::vector<Vertex>& meshVertices, std::vector<unsigned int>& meshIndices, std::vector<MeshBone>& meshBones);

		glm::mat4		aiMatrix4x4ToGlm(const aiMatrix4x4& from);
		aiMatrix4x4		glmToAssImpMat4(glm::mat4 mat);

		

	public:
		AssimpMeshLoader();
		~AssimpMeshLoader() {};

		const std::vector<std::string>&		preload(const std::string& filePath);
		std::vector<std::unique_ptr<Mesh>>	load(const std::vector<std::string>* loadables);

		void reset();
	};
}