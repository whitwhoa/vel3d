#include <iostream>

#include "vel/Log.h"
#include "vel/Mesh.h"


namespace vel
{

    Mesh::Mesh(std::string name) :
        name(name)
    {}

	AABB& Mesh::getAABB()
	{
		if (this->aabb.has_value())
			return this->aabb.value();

		this->aabb = AABB(this->getVertices());

		return this->aabb.value();
	}

	const std::vector<MeshBone>& Mesh::getBones() const
	{
		return this->bones;
	}

	MeshBone* Mesh::getBone(std::string boneName)
	{
		for (auto& b : this->bones)
		{
			if (b.name == boneName)
				return &b;
		}
		return nullptr;
	}

	MeshBone& Mesh::getBone(size_t index)
	{
		return this->bones.at(index);
	}

	glm::mat4 Mesh::getGlobalInverseMatrix()
	{
		return this->globalInverseMatrix;
	}

	void Mesh::setGlobalInverseMatrix(glm::mat4 gim)
	{
		this->globalInverseMatrix = gim;
	}

	const bool Mesh::hasBones() const
	{
		if (this->bones.size() > 0)
		{
			return true;
		}
		return false;
	}

	void Mesh::addVertexWeight(unsigned int vertexIndex, unsigned int boneIndex, float weight)
	{
		// wtff???...oh ok, this just looks weird, but it's looping through each index in the raw array
		for (unsigned int i = 0; i < (sizeof(this->vertices[vertexIndex].weights.ids) / sizeof(this->vertices[vertexIndex].weights.ids[0])); i++)
		{
			if (this->vertices[vertexIndex].weights.weights[i] == 0.0f)
			{
				this->vertices[vertexIndex].weights.ids[i] = boneIndex;
				this->vertices[vertexIndex].weights.weights[i] = weight;
				return;
			}
		}		
	}

	void Mesh::setVertices(std::vector<Vertex>& vertices)
	{
		this->vertices = vertices;
	}

	void Mesh::setIndices(std::vector<unsigned int>& indices)
	{
		this->indices = indices;
	}

	void Mesh::setBones(std::vector<MeshBone>& bones)
	{
		this->bones = bones;
	}

    void Mesh::setGpuMesh(GpuMesh gm)
    {
		this->gpuMesh = gm;
    }

    std::vector<Vertex>& Mesh::getVertices()
    {
        return this->vertices;
    }

    std::vector<unsigned int>& Mesh::getIndices()
    {
        return this->indices;
    }

    const std::string Mesh::getName() const
    {
        return this->name;
    }

    const bool Mesh::isRenderable() const
    {
        return this->gpuMesh.has_value();
    }

    std::optional<GpuMesh>& Mesh::getGpuMesh()
    {
        return this->gpuMesh;
    }

	// utility function for when we use a mesh object to store a group of vertices
	// for the purpose of generating an AABB
	void Mesh::appendVertices(const std::vector<Vertex>& vs)
	{
		this->vertices.reserve(this->vertices.size() + vs.size());
		this->vertices.insert(this->vertices.end(), vs.begin(), vs.end());
	}

	void Mesh::initBillboardQuad(float width, float height)
	{
		if (!(width > 0.0f && height > 0.0f))
			Log::crash("Billboard width and height must be positive.");

		const float halfWidth = width * 0.5f;
		const float halfHeight = height * 0.5f;

		// Front face is toward -Z (matches billboard code using forward = -dir)
		const glm::vec3 n(0.0f, 0.0f, -1.0f);

		Vertex v0;
		v0.position = glm::vec3(-halfWidth, halfHeight, 0.0f);
		v0.normal = n;
		v0.textureCoordinates = glm::vec2(0.0f, 1.0f);
		v0.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v0.materialUBOIndex = 0;

		Vertex v1;
		v1.position = glm::vec3(-halfWidth, -halfHeight, 0.0f);
		v1.normal = n;
		v1.textureCoordinates = glm::vec2(0.0f, 0.0f);
		v1.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v1.materialUBOIndex = 0;

		Vertex v2;
		v2.position = glm::vec3(halfWidth, -halfHeight, 0.0f);
		v2.normal = n;
		v2.textureCoordinates = glm::vec2(1.0f, 0.0f);
		v2.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v2.materialUBOIndex = 0;

		Vertex v3;
		v3.position = glm::vec3(halfWidth, halfHeight, 0.0f);
		v3.normal = n;
		v3.textureCoordinates = glm::vec2(1.0f, 1.0f);
		v3.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v3.materialUBOIndex = 0;

		std::vector<Vertex> vs = { v0, v1, v2, v3 };
		this->setVertices(vs);

		// Flip indices so -Z is front (CCW when viewed from -Z)
		std::vector<unsigned int> is = { 0, 2, 1, 0, 3, 2 };
		this->setIndices(is);
	}

}