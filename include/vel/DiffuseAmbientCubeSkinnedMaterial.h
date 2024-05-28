#pragma once

#include "vel/Material.h"
#include "vel/SkinnedMaterialMixin.h"
#include "vel/AmbientCubeMaterialMixin.h"

namespace vel
{
	class DiffuseAmbientCubeSkinnedMaterial : public Material, public AmbientCubeMaterialMixin, public SkinnedMaterialMixin
	{
	public:
		static std::vector<std::string> shaderDefs;

		DiffuseAmbientCubeSkinnedMaterial(const std::string& name, Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		virtual std::unique_ptr<Material> clone() const override;
	};
}