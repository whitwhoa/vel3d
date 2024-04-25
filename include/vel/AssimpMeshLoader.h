#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>


#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"


#include "vel/Mesh.h"
#include "vel/Armature.h"
#include "vel/VertexBoneData.h"
#include "vel/Animation.h"


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
		std::vector<std::string>				meshesToLoad;
		std::string								armatureToLoad;
		std::vector<std::unique_ptr<Mesh>>		meshes;
		std::unique_ptr<Armature>				armature;
		unsigned int							currentMeshTextureId;
		std::vector<aiNode*>					processedNodes;
		std::vector<VertexBoneData>				currentMeshBones;
		glm::mat4								currentGlobalInverseMatrix;


		void			preProcessNode(aiNode* node);

		void			processAnimations();
		void			processArmatureNode(aiNode* node);
		void			processNode(aiNode* node);
		void			processMesh(aiMesh* aiMesh, std::vector<Vertex>& meshVertices, std::vector<unsigned int>& meshIndices, std::vector<MeshBone>& meshBones);
		bool			isRootArmatureNode(aiNode* node);
		glm::mat4		aiMatrix4x4ToGlm(const aiMatrix4x4& from);
		aiMatrix4x4		glmToAssImpMat4(glm::mat4 mat);
		bool			nodeHasBeenProcessed(aiNode* in);

		

	public:
		AssimpMeshLoader();
		~AssimpMeshLoader() {};

		std::pair<std::vector<std::string>, std::string> 
			preload(const std::string& filePath);

		std::pair<std::vector<std::unique_ptr<Mesh>>, std::unique_ptr<Armature>> 
			load(const std::pair<const std::vector<std::string>, const std::string>& loadables);

		void reset();
	};
}