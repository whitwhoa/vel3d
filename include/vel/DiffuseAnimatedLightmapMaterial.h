#pragma once

#include "vel/AnimatedMaterial.h"
#include "vel/LightmapMaterialMixin.h"

namespace vel
{
	class DiffuseAnimatedLightmapMaterial : public AnimatedMaterial, public LightmapMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseAnimatedLightmapMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
	};
}