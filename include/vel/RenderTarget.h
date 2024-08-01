#pragma once

#include "glm/glm.hpp"

#include "vel/Texture.h"

namespace vel
{
	struct RenderTarget
	{
		glm::ivec2 resolution;

		unsigned int opaqueFBO;
		unsigned int alphaFBO;

		Texture opaqueTexture;
		Texture depthTexture;

		Texture accumTexture;
		Texture revealTexture;
	};
}