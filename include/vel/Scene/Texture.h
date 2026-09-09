#pragma once

#include <string>
#include <vector>

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
		unsigned int	flags = 0;

		uint64_t		dsaHandle;
		unsigned int	bufferId; // used to be: id

		int				width = 0;
		int				height = 0;
		int				channels = 0; // used to be: nrComponents
		unsigned int	format = 0;
		unsigned int	sizedFormat = 0;

		unsigned char*	data = nullptr;
	};
}