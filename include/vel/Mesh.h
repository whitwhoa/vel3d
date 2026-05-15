#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>

#include "glm/glm.hpp"

#include "vel/Shader.h"
#include "vel/Vertex.h"
#include "vel/Texture.h"
#include "vel/GpuMesh.h"
#include "vel/MeshBone.h"
#include "vel/AABB.h"


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