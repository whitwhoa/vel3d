#pragma once

#include <string>
#include <optional>
#include <vector>

#include "glm/glm.hpp"

#include "vel/Texture.h"
#include "vel/MaterialAnimator.h"


namespace vel
{
	class Material
	{
	private:
		std::string							name;
		std::vector<Texture*>				textures;
		bool								hasAlphaChannel;

		std::optional<MaterialAnimator>		materialAnimator;

	public:
		Material(std::string name);

		void								addTexture(Texture* t);
		void								addAnimatedTexture(Texture* t, float fps);

		void								pauseAnimatedTextureAfterCycles(unsigned int textureId, unsigned int cycles);
		void								setAnimatedTexturePause(unsigned int textureId, bool isPaused);
		bool								getAnimatedTexturePause(unsigned int textureId);

		void								setAnimatedTextureReverse(unsigned int textureId, bool reverse);
		bool								getAnimatedTextureReversed(unsigned int textureId);

		const std::string&					getName() const;
		std::vector<Texture*>&				getTextures();
		std::optional<MaterialAnimator>&	getMaterialAnimator();
		bool								getHasAlphaChannel();
	};
}