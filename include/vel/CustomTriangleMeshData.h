#pragma once

#include "glm/glm.hpp"

#include "vel/Texture.h"

namespace vel
{
	struct CustomTriangleMeshData
	{
		glm::vec2 lightmapUVs;
		Texture* lightmapTexture; // image data values only stay in ram if denoted when loading textures
	};
}