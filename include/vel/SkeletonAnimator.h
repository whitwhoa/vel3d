#pragma once

#include <vector>


#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/containers/vector.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"

namespace vel
{
	enum class AnimUpdateMode 
	{ 
		SimInterpolated, 
		SimStep,
		RenderOnly 
	};

	class SkeletonAnimator
	{
	private:
		const ozz::animation::Skeleton*				skeleton;
		AnimUpdateMode								mode;

		// TODO: Samplers
		// TODO: Additives

		// Depending on AnimUpdateMode, we either utilize localsA/B and the double buffer
		// technique to then later interpolate values into finalLocalTransforms, or use
		// finalLocalTransforms directly

		ozz::vector<ozz::math::SoaTransform>		localsA;
		ozz::vector<ozz::math::SoaTransform>		localsB;
		ozz::vector<ozz::math::SoaTransform>*		simCurr;
		ozz::vector<ozz::math::SoaTransform>*		simPrev;
		ozz::vector<ozz::math::Float4x4>			simModels;


		ozz::vector<ozz::math::SoaTransform>		renderLocals;
		ozz::vector<ozz::math::Float4x4>			renderModels;

		bool		sampleInto(ozz::vector<ozz::math::SoaTransform>& dst);

	public:
		SkeletonAnimator(ozz::animation::Skeleton* skeleton, AnimUpdateMode mode = AnimUpdateMode::RenderOnly);

		

		void			logicUpdate(float dt);
		void			renderUpdate(float dt);



	};
}