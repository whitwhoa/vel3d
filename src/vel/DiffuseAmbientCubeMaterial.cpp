#include "vel/DiffuseAmbientCubeMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseAmbientCubeMaterial::shaderDefs = {"USE_AMBIENT_CUBE"};

	DiffuseAmbientCubeMaterial::DiffuseAmbientCubeMaterial(const std::string& name, Shader* shader) :
		Material(name, shader),
		AmbientCubeMaterialMixin()
	{}

	void DiffuseAmbientCubeMaterial::preDraw(float frameTime) {};

	void DiffuseAmbientCubeMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		// TODO: these state switches MUST be refactored once we get everything else organized and working as it should be again
		gpu->useShader(this->getShader());
		gpu->useMesh(actor->getMesh());

		gpu->setActiveMaterial(this);

		for (unsigned int i = 0; i < this->getTextures().size(); i++)
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(0).dsaHandle);

		gpu->setShaderVec3Array("ambientCube", this->getAmbientCube());

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseAmbientCubeMaterial::clone() const
	{
		return std::make_unique<DiffuseAmbientCubeMaterial>(*this);
	}
}