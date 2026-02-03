
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"

#include "vel/SkeletonAnimator.h"

namespace vel
{
	SkeletonAnimator::SkeletonAnimator(ozz::animation::Skeleton* skeleton, AnimUpdateMode mode) :
		skeleton(skeleton),
		mode(mode)
	{
		const int num_soa = skeleton->num_soa_joints();
		this->localsA.resize(num_soa);
		this->localsB.resize(num_soa);
		this->finalLocals.resize(num_soa);

		this->finalModels.resize(skeleton->num_joints());

		this->simPrev = &this->localsA;
		this->simCurr = &this->localsB;

		// TODO: prime transform vecs, but first we must implement the logic that will perform the animations
		// taking into account all of the required logic
	}
	
	bool SkeletonAnimator::sampleInto(ozz::vector<ozz::math::SoaTransform>& dst)
	{
		ozz::animation::SamplingJob job;
		job.animation = animation;
		job.context = &ctx;
		job.ratio = controller.time_ratio();
		job.output = make_span(dst);

		return job.Run();
	}

	void SkeletonAnimator::logicUpdate(float dt)
	{

	}

	void SkeletonAnimator::renderUpdate(float dt)
	{

	}
}