#pragma once

#include <string>
#include <vector>

#include <vel/Scene/Texture/TextureData.h>

namespace vel
{
	enum TextureFlags
	{
		TXT_OPT_NONE = 0,
		TXT_OPT_CLAMP_UVS = 1 << 1,
		TXT_OPT_CPU_AND_GPU = 1 << 2,
		TXT_OPT_DISABLE_FILTER = 1 << 3
	};

	struct Texture
	{
		std::vector<TextureData>	frames;
		unsigned int				flags = 0;
	};
}