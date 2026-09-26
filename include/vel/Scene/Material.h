#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include <vel/Scene/Texture.h>


namespace vel
{
	typedef unsigned int material_handle;

	enum MtlFlg : uint32_t
	{
		MTLFLG_NONE = 0,
		MTLFLG_IS_TRANSPARENT = 1 << 0, // used to be HAS_ALPHA
		MTLFLG_IS_ALPHA_CUTOUT = 1 << 1, // used to be IS_CUTOUT
		MTLFLG_IS_ALPHA_MASK = 1 << 2,
		MTLFLG_IS_SKINNED = 1 << 3,
		MTLFLG_IS_RGB = 1 << 4,
		MTLFLG_IS_RGBA = 1 << 5,
		MTLFLG_IS_TEXT = 1 << 6,
		MTLFLG_IS_LINE = 1 << 7,

		MTLFLG_HAS_LIGHTMAP = 1 << 8,
		MTLFLG_HAS_TEXTURES = 1 << 9,
		MTLFLG_HAS_AMBIENT_CUBE = 1 << 10
	};

	struct Material
	{
		std::vector<texture_handle> textures;

		float f1 = 0.0f; // IE: lineThickness...etc
		float f2 = 0.0f;

		uint32_t flags = 0;
		uint32_t shaderProgramId = 0;
	};
}