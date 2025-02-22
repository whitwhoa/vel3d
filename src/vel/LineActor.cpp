

#include "vel/RGBALineMaterial.h"
#include "vel/Mesh.h"
#include "vel/LineActor.h"


namespace vel
{
	LineActor::LineActor(const std::string& name) :
		name(name),
		actor(nullptr),
		requiresUpdate(false)
	{}

	std::unique_ptr<Mesh> LineActor::pointsToMesh(const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points)
	{
		std::vector<Vertex> meshVertices = {};

		for (auto& p : points)
		{
			Vertex v1;
			v1.position = glm::vec3(std::get<0>(p), 0.0f);
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoordinates = glm::vec2(0.0, 0.0);
			v1.materialUBOIndex = std::get<2>(p);
			meshVertices.push_back(v1);

			Vertex v2;
			v2.position = glm::vec3(std::get<1>(p), 0.0f);
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoordinates = glm::vec2(0.0, 0.0);
			v2.materialUBOIndex = std::get<2>(p);
			meshVertices.push_back(v2);
		}

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(name + "_mesh");
		m->setVertices(meshVertices);
		
		return m;
	}

	void LineActor::setThickness(float t)
	{
		reinterpret_cast<RGBALineMaterial*>(this->actor->getMaterial())->setLineThickness(t);
	}


}
