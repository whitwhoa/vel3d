#include "vel/Shader.h"
#include "vel/Material.h"


namespace vel
{

	Material::Material(const std::string& name, Shader* shader) :
		name(name),
		shader(shader),
		color(glm::vec4(1.0f)),
		hasAlphaChannel(false)
	{}

	const std::string& Material::getName() const
	{
		return this->name;
	}

	bool Material::getHasAlphaChannel() const
	{
		return this->hasAlphaChannel;
	}

	void Material::setHasAlphaChannel(bool b)
	{
		this->hasAlphaChannel = b;
	}

	void Material::addTexture(Texture* t)
	{
		this->textures.push_back(t);

		if (t->alphaChannel)
			this->hasAlphaChannel = true;
	}

	std::vector<Texture*>& Material::getTextures()
	{
		return this->textures;
	}

	void Material::setColor(glm::vec4 c)
	{
		this->color = c;
	}

	const glm::vec4& Material::getColor()
	{
		return this->color;
	}

	void Material::setShader(Shader* s)
	{
		this->shader = s;
	}

	Shader* Material::getShader()
	{
		return this->shader;
	}

}