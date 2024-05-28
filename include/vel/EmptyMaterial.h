#pragma once

#include "vel/Material.h"

namespace vel
{
	class EmptyMaterial : public Material
	{
	public:
		static std::vector<std::string> shaderDefs;

		EmptyMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		std::unique_ptr<Material> clone() const override;
	};
}