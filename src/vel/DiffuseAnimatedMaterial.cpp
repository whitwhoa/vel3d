#include "vel/DiffuseAnimatedMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseAnimatedMaterial::shaderDefs = {};

	DiffuseAnimatedMaterial::DiffuseAnimatedMaterial(const std::string& name, Shader* shader) :
		AnimatedMaterial(name, shader)
	{}

	void DiffuseAnimatedMaterial::preDraw(float frameTime) 
	{
		this->getMaterialAnimator().update(frameTime);
	};

	void DiffuseAnimatedMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		for (unsigned int i = 0; i < this->getTextures().size(); i++)
		{
			auto currentTextureFrame = this->getMaterialAnimator().getTextureCurrentFrame(i);
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(currentTextureFrame).dsaHandle);
		}


		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseAnimatedMaterial::clone() const
	{
		return std::make_unique<DiffuseAnimatedMaterial>(*this);
	}

}