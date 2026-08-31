#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vel/Scene/Stage/Actor/Mesh/Mesh.h>
#include <vel/Scene/Stage/Actor/Mesh/MeshLoaderInterface.h>


namespace vel
{
	class AssimpMeshLoader : public MeshLoaderInterface
	{
	private:
		Assimp::Importer									aiImporter;
		const aiScene*										impScene;

		std::vector<std::pair<std::string, VtxLayout>>		preloadData; // <mesh name, vertex layout>

		std::string											armatureInFile;
		std::vector<std::pair<std::string, GeoPool*>>*		meshesToLoad;
		std::vector<std::unique_ptr<Mesh>>					meshes; // TF? this is a private member but it's later returned using std::move()
		

		void			preProcessNode(aiNode* node);

		void			processNode(aiNode* node);
		void			processMesh(aiMesh* aiMesh, Mesh* mesh);

		void			processVtxPos(aiMesh* aiMesh, Mesh* mesh);
		void			processVtxPosNrml(aiMesh* aiMesh, Mesh* mesh);
		void			processVtxPosNrmlTx(aiMesh* aiMesh, Mesh* mesh);
		void			processVtxPosNrmlTxLm(aiMesh* aiMesh, Mesh* mesh);
		void			processVtxPosNrmlTxSkn(aiMesh* aiMesh, Mesh* mesh, unsigned int offset);

		glm::mat4		aiMatrix4x4ToGlm(const aiMatrix4x4& from);

		

	public:
		AssimpMeshLoader();
		~AssimpMeshLoader() {};

		const std::vector<std::pair<std::string, VtxLayout>>& preload(const std::string& filePath) override;
		std::vector<std::unique_ptr<Mesh>> load(std::vector<std::pair<std::string, GeoPool*>>* loadables) override;

		void reset() override;

	};
}