#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>


namespace vel
{
	typedef unsigned int shader_handle;

	struct Shader
	{
		unsigned int programId = 0; // opengl program id, used to be called "id"
		std::string vertCode = "";
		std::string geomCode = "";
		std::string fragCode = "";
	};
}