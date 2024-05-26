#pragma once

#include "vel/Material.h"

namespace vel
{
	class DiffuseMaterial : public Material
	{
	public:
		static std::vector<std::string> shaderDefs;
		
		DiffuseMaterial(const std::string& name, Shader* shader);
		
		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
	};
}