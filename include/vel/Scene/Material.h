#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include <vel/Scene/Texture/Texture.h>


namespace vel
{
	struct Material
	{
		std::vector<Texture*> textures;
		std::vector<glm::vec4> lineColors;
		std::vector<glm::vec3> ambientCube;

		glm::vec4 color1 = glm::vec4(1.0f);
		glm::vec4 color2 = glm::vec4(1.0f);

		float f1 = 0.0f; // IE: causticStrength || lineThickness || etc
		float f2 = 0.0f;

		uint32_t flags = 0;
		uint32_t shaderId = 0;
	};
}