#pragma once

#include <vector>
#include <string>
#include <bitset>

#include "vel/Channel.h"


namespace vel
{
	struct Animation
	{
		std::string					name;
		float						duration;
		float						tps;
		std::vector<Channel>		channels; // each individual bone in the animation
		std::bitset<256>			channelMask; // channel blacklist, ie which bones we do not want to affect this animation
	};
	
}