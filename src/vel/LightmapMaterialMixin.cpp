#include "vel/Texture.h"
#include "vel/LightmapMaterialMixin.h"

namespace vel
{
	LightmapMaterialMixin::LightmapMaterialMixin() :
		lightmapTexture(nullptr)
	{}

	void LightmapMaterialMixin::setLightmapTexture(Texture* t)
	{
		this->lightmapTexture = t;
	}

	Texture* LightmapMaterialMixin::getLightmapTexture()
	{
		return this->lightmapTexture;
	}
}