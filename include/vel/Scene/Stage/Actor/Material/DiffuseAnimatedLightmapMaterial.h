#pragma once

#include <vel/Scene/Stage/Actor/Material/AnimatedMaterial.h>
#include <vel/Scene/Stage/Actor/Material/LightmapMaterialMixin.h>

namespace vel
{
	class DiffuseAnimatedLightmapMaterial : public AnimatedMaterial, public LightmapMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseAnimatedLightmapMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		virtual std::unique_ptr<Material> clone() const override;
	};
}