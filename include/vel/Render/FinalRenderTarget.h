#pragma once

#include <glm/glm.hpp>

#include <vel/Scene/Stage/Actor/Material/Texture/Texture.h>

namespace vel
{
	struct FinalRenderTarget
	{
		glm::ivec2 resolution;
		unsigned int fbo;
		Texture texture;
	};
}