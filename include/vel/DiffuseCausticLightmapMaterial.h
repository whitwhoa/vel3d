#pragma once

#include "glm/glm.hpp"

#include "vel/AnimatedMaterial.h"
#include "vel/LightmapMaterialMixin.h"

namespace vel
{
	class DiffuseCausticLightmapMaterial : public AnimatedMaterial, public LightmapMaterialMixin
	{
	private:
		glm::vec4 surfaceColor;
		float strength;

	public:
		static std::vector<std::string> shaderDefs;

		DiffuseCausticLightmapMaterial(const std::string& name, Shader* shader);

		void setSurfaceColor(glm::vec4 surfaceColor);
		void setCausticStrength(float strength);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		virtual std::unique_ptr<Material> clone() const override;
	};
}