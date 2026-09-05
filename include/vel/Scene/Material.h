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
		std::vector<glm::vec4> colors;

		float f1 = 0.0f; // IE: lineThickness...etc
		float f2 = 0.0f;

		uint32_t flags = 0;
		uint32_t shaderId = 0;
	};
}