#pragma once

#include "vel/Material.h"
#include "vel/LightmapMaterialMixin.h"

namespace vel
{
	class DiffuseLightmapMaterial : public Material, public LightmapMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseLightmapMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		virtual std::unique_ptr<Material> clone() const override;
	};
}