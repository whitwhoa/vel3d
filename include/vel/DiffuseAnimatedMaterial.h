#pragma once

#include "vel/AnimatedMaterial.h"

namespace vel
{
	class DiffuseAnimatedMaterial : public AnimatedMaterial
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseAnimatedMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
	};
}