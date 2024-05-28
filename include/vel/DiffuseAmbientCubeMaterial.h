#pragma once

#include "vel/Material.h"
#include "vel/AmbientCubeMaterialMixin.h"

namespace vel
{
	class DiffuseAmbientCubeMaterial : public Material, public AmbientCubeMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseAmbientCubeMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		std::unique_ptr<Material> clone() const override;
	};
}