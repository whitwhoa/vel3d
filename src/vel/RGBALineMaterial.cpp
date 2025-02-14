#include <iostream>

#include "vel/RGBALineMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> RGBALineMaterial::shaderDefs = { "RGBA_ONLY" };

	RGBALineMaterial::RGBALineMaterial(const std::string& name, Shader* shader) :
		lineThickness(1.0f),
		Material(name, shader)
	{}

	void RGBALineMaterial::setLineThickness(float lt)
	{
		this->lineThickness = lt;
	}

	void RGBALineMaterial::preDraw(float frameTime) {};

	void RGBALineMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		gpu->setShaderFloat("uLineWidth", this->lineThickness);
		gpu->setShaderFloat("uViewportHeight", (float)gpu->getActiveViewportSize().y);
		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		//gpu->drawGpuMesh();
		gpu->drawLines(actor->getMesh()->getVertices().size());

	}

	std::unique_ptr<Material> RGBALineMaterial::clone() const
	{
		return std::make_unique<RGBALineMaterial>(*this);
	}
}