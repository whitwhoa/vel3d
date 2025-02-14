#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "glm/glm.hpp"


typedef int GLint;

namespace vel
{
	struct Shader
	{
		unsigned int id;
		std::string name;
		std::string vertCode;
		std::string geomCode;
		std::string fragCode;
		std::unordered_map<std::string, GLint> uniformLocations;
	};
}