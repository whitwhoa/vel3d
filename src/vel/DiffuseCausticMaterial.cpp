#include "vel/DiffuseCausticMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> DiffuseCausticMaterial::shaderDefs = {"IS_CAUSTIC"};

	DiffuseCausticMaterial::DiffuseCausticMaterial(const std::string& name, Shader* shader) :
		surfaceColor(glm::vec4(1.0f)),
		strength(1.0f),
		AnimatedMaterial(name, shader)
	{}

	void DiffuseCausticMaterial::preDraw(float frameTime)
	{
		this->getMaterialAnimator().update(frameTime);
	};

	void DiffuseCausticMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		for (unsigned int i = 0; i < this->getTextures().size(); i++)
		{
			auto currentTextureFrame = this->getMaterialAnimator().getTextureCurrentFrame(i);
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(currentTextureFrame).dsaHandle);
		}
		

		gpu->setShaderVec4("color", this->getColor());

		gpu->setShaderVec4("surfaceColor", this->surfaceColor);
		gpu->setShaderFloat("causticStrength", this->strength);

		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> DiffuseCausticMaterial::clone() const
	{
		return std::make_unique<DiffuseCausticMaterial>(*this);
	}

	void DiffuseCausticMaterial::setSurfaceColor(glm::vec4 surfaceColor)
	{
		this->surfaceColor = surfaceColor;
	}

	void DiffuseCausticMaterial::setCausticStrength(float strength)
	{
		this->strength = strength;
	}


}