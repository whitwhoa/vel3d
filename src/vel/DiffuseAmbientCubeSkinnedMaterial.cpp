#include "vel/DiffuseAmbientCubeSkinnedMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseAmbientCubeSkinnedMaterial::shaderDefs = { "IS_SKINNED", "USE_AMBIENT_CUBE"};

	DiffuseAmbientCubeSkinnedMaterial::DiffuseAmbientCubeSkinnedMaterial(const std::string& name, Shader* shader) :
		Material(name, shader)
	{}

	void DiffuseAmbientCubeSkinnedMaterial::preDraw(float frameTime) {};

	void DiffuseAmbientCubeSkinnedMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		for (unsigned int i = 0; i < this->getTextures().size(); i++)
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(0).dsaHandle);

		this->updateBones(alphaTime, gpu, actor);

		gpu->setShaderVec3Array("ambientCube", this->getAmbientCube());

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseAmbientCubeSkinnedMaterial::clone() const
	{
		return std::make_unique<DiffuseAmbientCubeSkinnedMaterial>(*this);
	}

}