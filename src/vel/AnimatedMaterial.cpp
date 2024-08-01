#include "vel/Shader.h"
#include "vel/AnimatedMaterial.h"

namespace vel
{
	AnimatedMaterial::AnimatedMaterial(const std::string& name, Shader* shader) :
		materialAnimator(MaterialAnimator()),
		Material(name, shader) 
	{}

	void AnimatedMaterial::addAnimatedTexture(Texture* t, float fps)
	{
		this->addTexture(t);
		this->materialAnimator.addTextureAnimator(t->frames.size(), fps);

		//if (t->alphaChannel)
		//	this->setHasAlphaChannel(true);
	}

	void AnimatedMaterial::pauseAnimatedTextureAfterCycles(unsigned int textureId, unsigned int cycles)
	{
		this->materialAnimator.setTextureAnimatorPauseAfterCycles(textureId, cycles);
	}

	void AnimatedMaterial::setAnimatedTexturePause(unsigned int textureId, bool isPaused)
	{
		this->materialAnimator.setTextureAnimatorPause(textureId, isPaused);
	}

	bool AnimatedMaterial::getAnimatedTexturePause(unsigned int textureId)
	{
		return this->materialAnimator.getTexturePaused(textureId);
	}

	void AnimatedMaterial::setAnimatedTextureReverse(unsigned int textureId, bool reverse)
	{
		this->materialAnimator.setAnimatedTextureReverse(textureId, reverse);
	}

	bool AnimatedMaterial::getAnimatedTextureReversed(unsigned int textureId)
	{
		return this->materialAnimator.getAnimatedTextureReversed(textureId);
	}

	MaterialAnimator& AnimatedMaterial::getMaterialAnimator()
	{
		return this->materialAnimator;
	}

}