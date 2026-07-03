
#include "vel/DiffuseSingleSelectableMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseSingleSelectableMaterial::shaderDefs = {};

	DiffuseSingleSelectableMaterial::DiffuseSingleSelectableMaterial(const std::string& name, Shader* shader) :
		Material(name, shader),
		activeIndex(0)
	{}

	void DiffuseSingleSelectableMaterial::preDraw(float frameTime) {};

	void DiffuseSingleSelectableMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		gpu->updateTextureUBO(0, this->getTextures()[this->activeIndex]->frames[0].dsaHandle);
		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseSingleSelectableMaterial::clone() const
	{
		return std::make_unique<DiffuseSingleSelectableMaterial>(*this);
	}
}