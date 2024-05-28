#include "vel/AmbientCubeMaterialMixin.h"

namespace vel
{
	AmbientCubeMaterialMixin::AmbientCubeMaterialMixin() :
		ambientCube(std::vector<glm::vec3>{ glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) })
	{}

	std::vector<glm::vec3>& AmbientCubeMaterialMixin::getAmbientCube()
	{
		return this->ambientCube;
	}

	void AmbientCubeMaterialMixin::updateAmbientCube(std::vector<glm::vec3>& colors)
	{
		this->ambientCube = colors;
	}
	
}