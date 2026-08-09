#pragma once

#include <vel/Scene/Stage/Actor/Material/AnimatedMaterial.h>

namespace vel
{
	class AnimatedBillboardMaterial : public AnimatedMaterial
	{
	private:
		int activeTexture;

	public:
		static std::vector<std::string> shaderDefs;
		AnimatedBillboardMaterial(const std::string& name, vel::Shader* shader);

		void preDraw(float frameTime) override;
		void draw(float alphaTime, vel::GPU* gpu, vel::Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) override;
		virtual std::unique_ptr<Material> clone() const override;


		int getActiveTexture();
		void setActiveTexture(int t);


	};
}