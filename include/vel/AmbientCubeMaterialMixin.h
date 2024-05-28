#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace vel
{
	class AmbientCubeMaterialMixin
	{
	private:
		std::vector<glm::vec3> ambientCube; // -x, +x, -y, +y, -z, +z

	public:
		AmbientCubeMaterialMixin();

		std::vector<glm::vec3>&	getAmbientCube();
		void					updateAmbientCube(std::vector<glm::vec3>& colors);

	};
}