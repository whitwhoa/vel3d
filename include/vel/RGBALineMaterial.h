#pragma once

#include "glm/glm.hpp"

#include "vel/Material.h"

namespace vel
{
	class RGBALineMaterial : public Material
	{
	private:
		float lineThickness;
		glm::mat4 lineColors;

	public:
		static std::vector<std::string> shaderDefs;

		RGBALineMaterial(const std::string& name, Shader* shader);

		void setLineThickness(float lt);

		// int i can be from 0 to 3 | ie up to 4 colors per material since we store them in a mat4
		void setLineColor(int i, glm::vec4 c);


		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		std::unique_ptr<Material> clone() const override;
	};
}