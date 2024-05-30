#include "vel/DiffuseSkinnedMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseSkinnedMaterial::shaderDefs = {"IS_SKINNED"};

	DiffuseSkinnedMaterial::DiffuseSkinnedMaterial(const std::string& name, Shader* shader) :
		Material(name, shader)
	{}

	void DiffuseSkinnedMaterial::preDraw(float frameTime) {};

	void DiffuseSkinnedMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		for (unsigned int i = 0; i < this->getTextures().size(); i++)
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(0).dsaHandle);

		this->updateBones(alphaTime, gpu, actor);

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseSkinnedMaterial::clone() const
	{
		return std::make_unique<DiffuseSkinnedMaterial>(*this);
	}

}