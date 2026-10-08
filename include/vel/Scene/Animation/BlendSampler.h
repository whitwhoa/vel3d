#pragma once

#include <ozz/animation/runtime/sampling_job.h>

#include <vel/Scene/Animation/SkelAnimator.h>
#include <vel/Scene/Animation/SkelAnimController.h>


namespace vel
{
	struct BlendSampler
	{
		vel::SkelAnimController                 controller;
		float                                   weight = 1.f;
		ozz::animation::Animation*              animation = nullptr;
		ozz::animation::SamplingJob::Context    context;
		ozz::vector<ozz::math::SoaTransform>    locals;
		//ozz::vector<ozz::math::SimdFloat4>      jointWeights;


		BlendSampler() {};
	};
}
