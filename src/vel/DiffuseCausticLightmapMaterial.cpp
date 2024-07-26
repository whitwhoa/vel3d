#include "vel/DiffuseCausticLightmapMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseCausticLightmapMaterial::shaderDefs = { "IS_CAUSTIC", "USE_LIGHTMAP"};

	DiffuseCausticLightmapMaterial::DiffuseCausticLightmapMaterial(const std::string& name, Shader* shader) :
		surfaceColor(glm::vec4(1.0f)),
		strength(1.0f),
		AnimatedMaterial(name, shader),
		LightmapMaterialMixin()
	{}

	void DiffuseCausticLightmapMaterial::preDraw(float frameTime)
	{
		this->getMaterialAnimator().update(frameTime);
	};

	void DiffuseCausticLightmapMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		for (unsigned int i = 0; i < this->getTextures().size(); i++)
		{
			auto currentTextureFrame = this->getMaterialAnimator().getTextureCurrentFrame(i);
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(currentTextureFrame).dsaHandle);
		}

		gpu->updateLightmapTextureUBO(this->getLightmapTexture()->frames.at(0).dsaHandle);

		gpu->setShaderVec4("color", this->getColor());

		gpu->setShaderVec4("surfaceColor", this->surfaceColor);
		gpu->setShaderFloat("causticStrength", this->strength);

		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseCausticLightmapMaterial::clone() const
	{
		return std::make_unique<DiffuseCausticLightmapMaterial>(*this);
	}

	void DiffuseCausticLightmapMaterial::setSurfaceColor(glm::vec4 surfaceColor)
	{
		this->surfaceColor = surfaceColor;
	}

	void DiffuseCausticLightmapMaterial::setCausticStrength(float strength)
	{
		this->strength = strength;
	}


}