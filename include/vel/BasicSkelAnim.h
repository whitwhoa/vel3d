#pragma once

#include "ozz/animation/runtime/sampling_job.h"

#include "vel/SkelAnimator.h"
#include "vel/SkelAnimController.h"


/*
	More or less intended to be a basic example of how to extend SkelAnimator and supply required methods that
	utilize ozz
*/

namespace vel
{
	class BasicSkelAnim : public SkelAnimator
	{
	private:
		SkelAnimController controller;
		ozz::animation::Animation* animation;
		ozz::animation::SamplingJob::Context context;

	public:
		BasicSkelAnim(ozz::animation::Skeleton* skeleton);

		void setAnimation(ozz::animation::Animation* a);

		bool init() override;
		bool onUpdate(float logicTick) override;
	};
}