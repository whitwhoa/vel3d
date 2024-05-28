#pragma once

#include <memory>
#include "vel/Material.h"
#include "vel/MaterialAnimator.h"

namespace vel
{
	class AnimatedMaterial : public Material
	{
	private:
		MaterialAnimator	materialAnimator;

	public:
		AnimatedMaterial(const std::string& name, Shader* shader);

		void				addAnimatedTexture(Texture* t, float fps);

		void				pauseAnimatedTextureAfterCycles(unsigned int textureId, unsigned int cycles);
		void				setAnimatedTexturePause(unsigned int textureId, bool isPaused);
		bool				getAnimatedTexturePause(unsigned int textureId);

		void				setAnimatedTextureReverse(unsigned int textureId, bool reverse);
		bool				getAnimatedTextureReversed(unsigned int textureId);

		MaterialAnimator&	getMaterialAnimator();


		virtual void preDraw(float frameTime) = 0;
		virtual void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) = 0;
		virtual std::unique_ptr<Material> clone() const = 0;

	};
}