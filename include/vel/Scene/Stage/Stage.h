#pragma once

#include <vector>

#include <vel/Scene/Camera/Camera.h>
#include <vel/Scene/Stage/DrawBucket/DrawBucket.h>

namespace vel
{
	struct Stage
	{
		bool enabled = true;

		std::vector<Camera*> cameras;
		std::vector<DrawBucket> opaqueBuckets;
		std::vector<DrawBucket> transparentBuckets;
	};
}