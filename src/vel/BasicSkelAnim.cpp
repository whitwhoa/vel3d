
#include "ozz/animation/runtime/local_to_model_job.h"

#include "vel/BasicSkelAnim.h"

namespace vel
{
	BasicSkelAnim::BasicSkelAnim(ozz::animation::Skeleton* skeleton) :
		SkelAnimator(skeleton),
		animation(nullptr)
	{}

	void BasicSkelAnim::setAnimation(ozz::animation::Animation* a)
	{
		this->animation = a;
	}

	bool BasicSkelAnim::init()
	{
		// Skeleton and animation needs to match.
		if (this->skeleton->num_joints() != this->animation->num_tracks())
			return false;

		// Allocates runtime buffers.
		const int numSoaJoints = this->skeleton->num_soa_joints();
		this->simLocalTransforms->resize(numSoaJoints);

		const int numJoints = this->skeleton->num_joints();
		this->simModelMatrices.resize(numJoints);

		// Allocate a context that matches animation requirements.
		this->context.Resize(numJoints);

		return true;
	}

	bool BasicSkelAnim::onUpdate(float logicTick)
	{
		// Updates current animation time.
		this->controller.update(*this->animation, logicTick);

		// Samples optimized animation
		ozz::animation::SamplingJob sj;
		sj.animation = this->animation;
		sj.context = &this->context;
		sj.ratio = this->controller.getTimeRatio();
		sj.output = make_span(*this->simLocalTransforms);

		if (!sj.Run())
			return false;

		// Converts from local space to model space matrices.
		ozz::animation::LocalToModelJob ltmj;
		ltmj.skeleton = this->skeleton;
		ltmj.input = make_span(*this->simLocalTransforms);
		ltmj.output = make_span(this->simModelMatrices);

		if (!ltmj.Run())
			return false;

		return true;
	}
}