#pragma once

#include "vel/Material.h"

namespace vel
{
	class RGBALineMaterial : public Material
	{
	private:
		float lineThickness;

	public:
		static std::vector<std::string> shaderDefs;

		RGBALineMaterial(const std::string& name, Shader* shader);

		void setLineThickness(float lt);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		std::unique_ptr<Material> clone() const override;
	};
}