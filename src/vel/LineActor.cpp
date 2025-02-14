

#include "vel/Mesh.h"
#include "vel/LineActor.h"

namespace vel
{
	LineActor::LineActor(const std::string& name) :
		name(name),
		actor(nullptr),
		requiresUpdate(false)
	{}

	std::unique_ptr<Mesh> LineActor::pointsToMesh(const std::string& name, std::vector<glm::vec2> points)
	{
		std::vector<Vertex> meshVertices = {};

		for (auto& p : points)
		{
			Vertex v1;
			v1.position = glm::vec3(p, 0.0f);
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoordinates = glm::vec2(0.0, 0.0);
			v1.materialUBOIndex = 0;
			meshVertices.push_back(v1);
		}

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(name + "_mesh");
		m->setVertices(meshVertices);
		
		return m;
	}


}
