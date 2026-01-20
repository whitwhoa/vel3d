#pragma once

#include <vector>
#include <string>

#include "vel/Channel.h"


namespace vel
{
	struct Animation
	{
		std::string				name;
		float					duration;
		float					tps;
		std::vector<Channel>	channels; // each individual bone in the animation
	};
	
}