#include <vel/Scene/Stage/Actor/Material/Texture/Texture.h>
#include <vel/Scene/Stage/Actor/Material/LightmapMaterialMixin.h>

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