#pragma once

#include <cstdint>

namespace vel
{
	struct MeshSection
	{
		uint32_t firstIndex = 0;
		uint32_t indexCount = 0;
		uint32_t materialIndex = 0;
	};
}