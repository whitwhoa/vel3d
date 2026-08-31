#pragma once

#include <cstdint>

namespace vel
{
	enum MeshFlag : uint32_t
	{
		MESHFLAG_NONE = 0,
		MESHFLAG_RENDERABLE = 1 << 0,
		MESHFLAG_POOLED = 1 << 1
	};
}