#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <vel/Scene/Texture.h>

namespace vel
{
	struct RenderTarget
	{
		glm::ivec2 resolution;

		uint32_t opaqueFBO;
		uint32_t alphaFBO;

		uint32_t opaqueBufferId;
		uint32_t depthBufferId;
		uint32_t accumBufferId;
		uint32_t revealBufferId;

		uint64_t opaqueDsaHandle;
		uint64_t depthDsaHandle;
		uint64_t accumDsaHandle;
		uint64_t revealDsaHandle;
	};
}