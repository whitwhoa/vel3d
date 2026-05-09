#pragma once

#include <string>
#include <vector>

#include "TextureData.h"

namespace vel
{
	enum TextureOptions
	{
		TXT_OPT_NONE = 0, // 0000
		TXT_OPT_HAS_ALPHA = 1 << 0, // 0001
		TXT_OPT_CLAMP_UVS = 1 << 1, // 0010
		TXT_OPT_CPU_AND_GPU = 1 << 2, // 0100
		TXT_OPT_DISABLE_FILTER = 1 << 3 // 1000
	};

	struct Texture
	{
		std::string					name;
		std::vector<TextureData>	frames;
		int							options;
	};
}