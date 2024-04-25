#pragma once

#include <unordered_map>
#include <string>

#include "vel/Channel.h"


namespace vel
{
	struct Animation
	{
		std::string				name;
		float					duration;
		float					tps;
		std::unordered_map<std::string, Channel> channels;
	};
	
}