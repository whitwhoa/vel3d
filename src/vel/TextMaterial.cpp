#include "vel/TextMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> TextMaterial::shaderDefs = {"IS_TEXT"};

	TextMaterial::TextMaterial(const std::string& name, Shader* shader) :
		Material(name, shader)
	{}

	void TextMaterial::preDraw(float frameTime) {};

	void TextMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		// TODO: these state switches MUST be refactored once we get everything else organized and working as it should be again
		gpu->useShader(this->getShader());
		gpu->useMesh(actor->getMesh());

		gpu->setActiveMaterial(this);

		for (unsigned int i = 0; i < this->getTextures().size(); i++)
			gpu->updateTextureUBO(i, this->getTextures().at(i)->frames.at(0).dsaHandle);



		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> TextMaterial::clone() const
	{
		return std::make_unique<TextMaterial>(*this);
	}

}