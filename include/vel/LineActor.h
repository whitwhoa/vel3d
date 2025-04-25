#pragma once



#include "vel/Actor.h"



namespace vel
{

	struct LineActor
	{
		std::string	name;
		Actor*		actor;
		bool		requiresUpdate;

		LineActor(const std::string& name);

		static std::unique_ptr<Mesh> segmentsToMesh(const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points);
		
		static std::unique_ptr<Mesh> pointsToMesh(const std::string& name, const std::vector<glm::vec2>& points);

		void setThickness(float t);

	};
}