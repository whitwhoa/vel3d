#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"


namespace vel
{
	// TODO: I bet this could be condensed into a single vector, with fixed strides, which
	// would likely improve cache usage
	struct Channel
	{
		std::vector<float> positionKeyTimes;
		std::vector<float> rotationKeyTimes;
		std::vector<float> scalingKeyTimes;
		std::vector<glm::vec3> positionKeyValues;
		std::vector<glm::quat> rotationKeyValues;
		std::vector<glm::vec3> scalingKeyValues;
	};	
}