#pragma once

#include "vel/Material.h"
#include "vel/SkinnedMaterialMixin.h"

namespace vel
{
	class DiffuseSkinnedMaterial : public Material, public SkinnedMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseSkinnedMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
	};
}