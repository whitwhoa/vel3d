#pragma once

#include "glm/glm.hpp"

#include "vel/Texture.h"

namespace vel
{
	struct FinalRenderTarget
	{
		glm::ivec2 resolution;
		unsigned int fbo;
		Texture texture;
	};
}