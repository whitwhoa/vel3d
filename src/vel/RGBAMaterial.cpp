#include "vel/RGBAMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> RGBAMaterial::shaderDefs = {"RGBA_ONLY"};

	RGBAMaterial::RGBAMaterial(const std::string& name, Shader* shader) :
		Material(name, shader)
	{}

	void RGBAMaterial::preDraw(float frameTime) {};

	void RGBAMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> RGBAMaterial::clone() const
	{
		return std::make_unique<RGBAMaterial>(*this);
	}
}