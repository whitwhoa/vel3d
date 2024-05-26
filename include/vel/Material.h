#pragma once

#include <string>
#include <optional>
#include <vector>

#include "glm/glm.hpp"

#include "vel/Texture.h"
#include "vel/MaterialAnimator.h"


namespace vel
{
	class GPU;
	class Shader;
	class Actor;

	class Material
	{
	private:
		std::string							name;
		glm::vec4							color;
		std::vector<Texture*>				textures;
		bool								hasAlphaChannel;
		Shader*								shader;

	public:
		Material(const std::string& name, Shader* shader);

		const std::string&			getName() const;
		bool						getHasAlphaChannel() const;
		void						setHasAlphaChannel(bool b);

		void						addTexture(Texture* t);
		std::vector<Texture*>&		getTextures();
		
		void						setColor(glm::vec4 c);
		const glm::vec4&			getColor();

		void						setShader(Shader* s);
		Shader*						getShader();


		virtual void preDraw(float frameTime) = 0;
		virtual void draw(float alphaTime, GPU* gpu, Actor* actor, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) = 0;
	};
}