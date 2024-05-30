#include "vel/RGBALightmapMaterial.h"
#include "vel/GPU.h"
#include "vel/Actor.h"

namespace vel
{
	std::vector<std::string> RGBALightmapMaterial::shaderDefs = { "RGBA_ONLY", "USE_LIGHTMAP" };

	RGBALightmapMaterial::RGBALightmapMaterial(const std::string& name, Shader* shader) :
		Material(name, shader),
		LightmapMaterialMixin()
	{}

	void RGBALightmapMaterial::preDraw(float frameTime) {};

	void RGBALightmapMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
	{
		gpu->updateLightmapTextureUBO(this->getLightmapTexture()->frames.at(0).dsaHandle);

		gpu->setShaderVec4("color", this->getColor());
		gpu->setShaderMat4("model", actor->getWorldRenderMatrix(alphaTime));
		gpu->setShaderMat4("view", viewMatrix);
		gpu->setShaderMat4("projection", projMatrix);
		gpu->drawGpuMesh();
	}

	std::unique_ptr<Material> RGBALightmapMaterial::clone() const
	{
		return std::make_unique<RGBALightmapMaterial>(*this);
	}
}