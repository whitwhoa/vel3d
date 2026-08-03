#include <vel/Render/GPU.h>
#include <vel/Scene/Stage/Actor/Actor.h>
#include <vel/Scene/Stage/Actor/Material/EmptyMaterial.h>

namespace vel
{
	std::vector<std::string> EmptyMaterial::shaderDefs = {};

	EmptyMaterial::EmptyMaterial(const std::string& name, Shader* shader) :
		Material(name, shader)
	{}

	void EmptyMaterial::preDraw(float frameTime) {};
	void EmptyMaterial::draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix){}
	std::unique_ptr<Material> EmptyMaterial::clone() const
	{
		return std::make_unique<EmptyMaterial>(*this);
	}
}