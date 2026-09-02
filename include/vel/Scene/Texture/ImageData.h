#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace vel
{
	struct ImageData
	{
		unsigned char*		data = nullptr;
		int					width = 0;
		int					height = 0;
		int					nrComponents = 0;
		unsigned int		format = 0;
		unsigned int		sizedFormat = 0;
	};
}