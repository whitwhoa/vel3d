#pragma once



#include <vel/Scene/Actor/Actor.h>



namespace vel
{

	struct Line
	{
		std::string	name;
		Actor*		actor;
		bool		requiresUpdate;

		Line(const std::string& name);

		static std::unique_ptr<Mesh> segmentsToMesh(const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points);
		
		static std::unique_ptr<Mesh> pointsToMesh(const std::string& name, const std::vector<glm::vec2>& points);

		void setThickness(float t);

	};
}