#include <vel/Render/GPU.h>
#include <vel/Scene/Stage/Actor/Actor.h>

#include <vel/Scene/Stage/Actor/Material/AnimatedBillboardMaterial.h>

namespace vel
{
	std::vector<std::string> AnimatedBillboardMaterial::shaderDefs = { "IS_CUTOUT" };

	AnimatedBillboardMaterial::AnimatedBillboardMaterial(const std::string& name, vel::Shader* shader) :
		activeTexture(0),
		AnimatedMaterial(name, shader)
	{
	}

	int AnimatedBillboardMaterial::getActiveTexture()
	{
		return this->activeTexture;
	}

	void AnimatedBillboardMaterial::setActiveTexture(int t)
	{
		this->activeTexture = t;
	}

	void AnimatedBillboardMaterial::preDraw(float frameTime)
	{
		this->getMaterialAnimator().update(frameTime);
	};

	void AnimatedBillboardMaterial::draw(float alphaTime, vel::GPU* gpu, vel::Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		unsigned int currentTextureFrame = this->getMaterialAnimator().getTextureCurrentFrame(this->activeTexture);
		gpu->updateTextureUBO(0, this->getTextures().at(this->activeTexture)->frames.at(currentTextureFrame).dsaHandle);

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<vel::Material> AnimatedBillboardMaterial::clone() const
	{
		return std::make_unique<AnimatedBillboardMaterial>(*this);
	}
}