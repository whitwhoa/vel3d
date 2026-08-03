#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>

#include <glm/glm.hpp>


#include <vel/Render/Shader.h>
#include <vel/Scene/Stage/Actor/Material/Texture/Texture.h>
#include <vel/Util/AABB.h>

#include <vel/Scene/Stage/Actor/Mesh/Vertex.h>
#include <vel/Scene/Stage/Actor/Mesh/GpuMesh.h>
#include <vel/Scene/Stage/Actor/Mesh/MeshBone.h>


namespace vel
{
	class Mesh
	{

	private:
		std::string                         name;
		std::vector<Vertex>					vertices;
		std::vector<unsigned int>           indices;
		std::vector<MeshBone>				bones;
		std::optional<GpuMesh>              gpuMesh;
		std::optional<AABB>					aabb;
		bool								aabbStale;


	public:
											Mesh(std::string name);
		void                                setGpuMesh(GpuMesh gm);
		void								setVertices(const std::vector<Vertex>& vertices);
		void								setIndices(const std::vector<unsigned int>& indices);
		void								setBones(const std::vector<MeshBone>& bones);
		std::optional<GpuMesh>&				getGpuMesh();
		const std::string                   getName() const;
		const std::vector<Vertex>&			getVertices() const;
		std::vector<Vertex>&				getMutableVertices();
		std::vector<unsigned int>&			getIndices();
		const bool                          isRenderable() const;
		const bool                          hasBones() const;
		MeshBone&							getBone(size_t index);
		MeshBone*							getBone(std::string boneName);
		const std::vector<MeshBone>&		getBones() const;

		AABB&								getAABB();

		void								appendVertices(const std::vector<Vertex>& vs);

		bool								initBillboardQuad(float width, float height);

	};
    
}