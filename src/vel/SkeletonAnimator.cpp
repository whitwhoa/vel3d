

#include "vel/SkeletonAnimator.h"

namespace vel
{
	SkeletonAnimator::SkeletonAnimator(ozz::animation::Skeleton* skeleton, bool isLogical = false) :
		skeleton(skeleton),
		logical(isLogical)
	{}
	
	unsigned int SkeletonAnimator::addAnimation(ozz::animation::Animation* a)
	{
		this->animations.push_back(a);
		return this->animations.size() - 1;
	}
}