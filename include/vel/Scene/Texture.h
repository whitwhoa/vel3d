#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vel
{
	typedef unsigned int texture_handle;

	enum TxtrFlg : uint32_t
	{
		TXTRFLG_NONE = 0,
		TXTRFLG_CLAMP_UVS = 1 << 1,
		TXTRFLG_CPU_AND_GPU = 1 << 2,
		TXTRFLG_FILTER = 1 << 3
	};

	struct Texture
	{
		uint32_t		flags = 0;

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