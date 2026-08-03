#pragma once

namespace vel
{
	class Texture;

	class LightmapMaterialMixin
	{
	private:
		Texture*	lightmapTexture;

	public:
		LightmapMaterialMixin();
		Texture*	getLightmapTexture();
		void		setLightmapTexture(Texture* t);

	};
}