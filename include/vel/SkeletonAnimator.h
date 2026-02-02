#pragma once

#include <vector>


#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/containers/vector.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"

namespace vel
{
	class SkeletonAnimator
	{
	private:
		bool logical;

	protected:
		ozz::animation::Skeleton* skeleton;
		std::vector<ozz::animation::Animation*> 	animations;

		ozz::vector<ozz::math::SoaTransform> 		localTransforms;
		ozz::vector<ozz::math::Float4x4> 			modelMatrices;

	public:
		SkeletonAnimator(ozz::animation::Skeleton* skeleton, bool isLogical = false);

		unsigned int addAnimation(ozz::animation::Animation* a);



	};
}