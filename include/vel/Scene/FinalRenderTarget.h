#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace vel
{
	struct FinalRenderTarget
	{
		glm::vec4 		colorMultiplier = glm::vec4(1.f, 1.f, 1.f, 0.f);
		glm::ivec2 		resolution = glm::ivec2(1280, 720);
		uint64_t		colorDsaHandle = 0;
		uint32_t 		fbo = 0;
		uint32_t		colorBufferId = 0;
		uint32_t		depthBufferId = 0;
	};
}