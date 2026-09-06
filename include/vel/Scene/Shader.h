#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>


typedef int GLint;

namespace vel
{
	struct Shader
	{
		unsigned int materialFlags = 0;
		unsigned int id = 0;
		std::string vertCode = "";
		std::string geomCode = "";
		std::string fragCode = "";
	};
}