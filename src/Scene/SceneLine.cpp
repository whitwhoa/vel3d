#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	/*
		Realized we didn't need Line type, because a Line was just an Actor with specific properties. This all needs refactored to account for that
		whenever we come back to refactor after we figure out how we're going to be handling materials and shaders
	*/

	Actor* Scene::addLine(const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors, float thickness)
	{
		// create the Line
		std::unique_ptr<Line> la = std::make_unique<Line>(name);

		// create the mesh
		Mesh* pMesh = this->addMesh(std::move(Line::segmentsToMesh(name, points)));

		bool hasAlpha = false;
		for (auto& c : colors)
		{
			if (c.w < 0.999f)
			{
				hasAlpha = true;
				break;
			}
		}

		// create material
		RGBALineMaterial* pMaterial = this->addRGBALineMaterial(name + "_material", hasAlpha);
		for (size_t i = 0; i < colors.size(); i++)
			pMaterial->setLineColor(i, colors[i]);

		// create actor
		Actor* pActor = stage->addActor(name, pMesh, pMaterial);

		// add actor pointer to Line
		la->actor = pActor;

		return stage->addLine(std::move(la));
	}

	Actor* Scene::addContinuousLine(const std::vector<glm::vec2>& points, glm::vec4 color, float thickness)
	{
		std::unique_ptr<Line> la = std::make_unique<Line>(name);

		Mesh* pMesh = this->addMesh(std::move(Line::pointsToMesh(name, points)));

		bool hasAlpha = color.w < 0.999f;

		RGBALineMaterial* pMaterial = this->addRGBALineMaterial(name + "_material", hasAlpha);
		pMaterial->setLineColor(0, color);

		Actor* pActor = stage->addActor(name, pMesh, pMaterial);

		la->actor = pActor;

		return stage->addLine(std::move(la));
	}

	std::unique_ptr<Mesh> Scene::linePointsToMesh(const std::vector<glm::vec2>& points)
	{
		std::vector<Vertex> meshVertices = {};

		for (int i = 0; i < points.size(); i++)
		{
			Vertex v1;
			v1.position = glm::vec3(points.at(i).x, points.at(i).y, 0.0f);
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoordinates = glm::vec2(0.0, 0.0);
			v1.materialUBOIndex = 0;
			meshVertices.push_back(v1);

			if (i == points.size() - 1)
				break;

			Vertex v2;
			v2.position = glm::vec3(points.at(i + 1).x, points.at(i + 1).y, 0.0f);
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoordinates = glm::vec2(0.0, 0.0);
			v2.materialUBOIndex = 0;
			meshVertices.push_back(v2);
		}

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(name + "_mesh");
		m->setVertices(meshVertices);

		return m;
	}

	std::unique_ptr<Mesh> Scene::lineSegmentsToMesh(const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points)
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


} // END NAMESPACE