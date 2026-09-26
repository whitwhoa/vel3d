#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <vel/Scene/Texture.h>

namespace vel
{
	struct RenderTarget
	{
		glm::ivec2 resolution		= glm::ivec2(0);

		uint32_t opaqueFBO			= 0;
		uint32_t alphaFBO			= 0;

		uint32_t opaqueBufferId		= 0;
		uint32_t depthBufferId		= 0;
		uint32_t accumBufferId		= 0;
		uint32_t revealBufferId		= 0;

		uint64_t opaqueDsaHandle	= 0;
		uint64_t depthDsaHandle		= 0;
		uint64_t accumDsaHandle		= 0;
		uint64_t revealDsaHandle	= 0;
	};
}