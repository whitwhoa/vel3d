#include "vel/Material.h"

namespace vel
{

	Material::Material(std::string name) :
		name(name),
		hasAlphaChannel(false)
	{}

	void Material::addTexture(Texture* t)
	{
		this->textures.push_back(t);

		if (t->alphaChannel)
			this->hasAlphaChannel = true;
	}

	void Material::addAnimatedTexture(Texture* t, float fps)
	{
		if (!this->materialAnimator.has_value())
			this->materialAnimator = MaterialAnimator();

		this->addTexture(t);
		this->materialAnimator->addTextureAnimator(t->frames.size(), fps);

		if (t->alphaChannel)
			this->hasAlphaChannel = true;
	}

	void Material::pauseAnimatedTextureAfterCycles(unsigned int textureId, unsigned int cycles)
	{
		if (!this->materialAnimator.has_value())
			return;

		this->materialAnimator.value().setTextureAnimatorPauseAfterCycles(textureId, cycles);
	}

	void Material::setAnimatedTexturePause(unsigned int textureId, bool isPaused)
	{
		if (!this->materialAnimator.has_value())
			return;

		this->materialAnimator.value().setTextureAnimatorPause(textureId, isPaused);
	}

	bool Material::getAnimatedTexturePause(unsigned int textureId)
	{
		if (!this->materialAnimator.has_value())
			return true;

		return this->materialAnimator.value().getTexturePaused(textureId);
	}

	void Material::setAnimatedTextureReverse(unsigned int textureId, bool reverse)
	{
		if (!this->materialAnimator.has_value())
			return;

		this->materialAnimator.value().setAnimatedTextureReverse(textureId, reverse);
	}

	bool Material::getAnimatedTextureReversed(unsigned int textureId)
	{
		if (!this->materialAnimator.has_value())
			return true;

		return this->materialAnimator.value().getAnimatedTextureReversed(textureId);
	}

	const std::string& Material::getName() const
	{
		return this->name;
	}

	std::vector<Texture*>& Material::getTextures()
	{
		return this->textures;
	}

	std::optional<MaterialAnimator>& Material::getMaterialAnimator()
	{
		return this->materialAnimator;
	}

	bool Material::getHasAlphaChannel()
	{
		return this->hasAlphaChannel;
	}

}