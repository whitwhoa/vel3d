#include "vel/DiffuseAnimatedLightmapMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseAnimatedLightmapMaterial::shaderDefs = { "USE_LIGHTMAP" };

	DiffuseAnimatedLightmapMaterial::DiffuseAnimatedLightmapMaterial(const std::string& name, Shader* shader) :
		AnimatedMaterial(name, shader),
		LightmapMaterialMixin()
	{}

	void DiffuseAnimatedLightmapMaterial::preDraw(float frameTime) 
	{
		this->getMaterialAnimator().update(frameTime);
	};

	void DiffuseAnimatedLightmapMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		// TODO: these state switches MUST be refactored once we get everything else organized and working as it should be again
		gpu->useShader(this->getShader());
		gpu->useMesh(actor->getMesh());

		gpu->setActiveMaterial(this);

		for (unsigned int i = 0; i < this->getTextures().size(); i++)
		{
			auto currentTextureFrame = this->getMaterialAnimator().getTextureCurrentFrame(i);
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(currentTextureFrame).dsaHandle);
		}

		gpu->updateLightmapTextureUBO(this->getLightmapTexture()->frames.at(0).dsaHandle);
		

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}
}