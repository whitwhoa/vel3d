
#include <string>

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"

#include "vel/SkelAnimator.h"

namespace vel
{
	SkelAnimator::SkelAnimator(ozz::animation::Skeleton* skeleton) :
		simTime(0.0f),
		skeleton(skeleton),
		simPrevLocalTransforms(&this->localTransformsA),
		simLocalTransforms(&this->localTransformsB)
	{
		// Allocate runtime buffers.
		this->localTransformsA.resize(this->skeleton->num_soa_joints());
		this->localTransformsB.resize(this->skeleton->num_soa_joints());
		this->renderLocalTransforms.resize(this->skeleton->num_soa_joints());

		this->simModelMatrices.resize(this->skeleton->num_joints());
		this->renderModelMatrices.resize(this->skeleton->num_joints());
	}

	ozz::math::SoaFloat3 SkelAnimator::lerpSoaFloat3(const ozz::math::SoaFloat3& a, const ozz::math::SoaFloat3& b, const ozz::math::SimdFloat4& t)
	{
		ozz::math::SoaFloat3 out;
		out.x = a.x + (b.x - a.x) * t;
		out.y = a.y + (b.y - a.y) * t;
		out.z = a.z + (b.z - a.z) * t;

		return out;
	}

	ozz::math::SoaQuaternion SkelAnimator::nLerpSoaQuaternion(const ozz::math::SoaQuaternion& a, ozz::math::SoaQuaternion b, const ozz::math::SimdFloat4& t)
	{
		// Dot product per lane
		const ozz::math::SimdFloat4 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

		// If dot < 0, negate b to take shortest path
		const ozz::math::SimdInt4 flip = ozz::math::CmpLt(dot, ozz::math::simd_float4::zero());

		b.x = ozz::math::Select(flip, -b.x, b.x);
		b.y = ozz::math::Select(flip, -b.y, b.y);
		b.z = ozz::math::Select(flip, -b.z, b.z);
		b.w = ozz::math::Select(flip, -b.w, b.w);

		// Nlerp
		ozz::math::SoaQuaternion q;
		q.x = a.x + (b.x - a.x) * t;
		q.y = a.y + (b.y - a.y) * t;
		q.z = a.z + (b.z - a.z) * t;
		q.w = a.w + (b.w - a.w) * t;

		// Normalize
		const ozz::math::SimdFloat4 len2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;

		const ozz::math::SimdFloat4 inv_len = ozz::math::simd_float4::one() / ozz::math::Sqrt(len2);

		q.x = q.x * inv_len;
		q.y = q.y * inv_len;
		q.z = q.z * inv_len;
		q.w = q.w * inv_len;

		return q;
	}

	void SkelAnimator::clearMask(ozz::vector<ozz::math::SimdFloat4>& weights, float value)
	{
		const ozz::math::SimdFloat4 v = value == 0.0f ? ozz::math::simd_float4::zero() : ozz::math::simd_float4::one();
		for (int i = 0; i < this->skeleton->num_soa_joints(); ++i)
			weights[i] = v;
	}

	float SkelAnimator::getSimTime() const
	{
		return this->simTime;
	}

	void SkelAnimator::update(float logicTick)
	{
		std::swap(simPrevLocalTransforms, simLocalTransforms);

		this->simTime += logicTick;

		this->onUpdate(logicTick);
	}

	void SkelAnimator::renderLerp(float alpha)
	{
		const int n = this->simPrevLocalTransforms->size();
		//this->renderLocalTransforms.resize(n);

		const ozz::math::SimdFloat4 t = ozz::math::simd_float4::Load1(alpha);
		const ozz::math::SoaTransform* prev = simPrevLocalTransforms->data();
		const ozz::math::SoaTransform* cur = simLocalTransforms->data();
		ozz::math::SoaTransform* out = renderLocalTransforms.data();

		for (int i = 0; i < n; ++i)
		{
			const ozz::math::SoaTransform& A = prev[i];
			const ozz::math::SoaTransform& B = cur[i];
			ozz::math::SoaTransform& O = out[i];

			O.translation = lerpSoaFloat3(A.translation, B.translation, t);
			O.scale = lerpSoaFloat3(A.scale, B.scale, t);
			O.rotation = nLerpSoaQuaternion(A.rotation, B.rotation, t);
		}

		ozz::animation::LocalToModelJob ltm;
		ltm.skeleton = this->skeleton;
		ltm.input = make_span(this->renderLocalTransforms);
		ltm.output = make_span(this->renderModelMatrices);
		ltm.Run();
	}

	const ozz::math::Float4x4& SkelAnimator::getSimBoneMatrix(unsigned int i)
	{
		return this->simModelMatrices.at(i);
	}

	const ozz::math::Float4x4& SkelAnimator::getRenderBoneMatrix(unsigned int i)
	{
		return this->renderModelMatrices.at(i);
	}

	int SkelAnimator::getBoneIndex(const std::string& name)
	{
		const auto& names = this->skeleton->joint_names();
		for (size_t i = 0; i < names.size(); ++i) 
			if (std::strcmp(names[i], name.c_str()) == 0) 
				return static_cast<int>(i);

		return -1;
	}

	std::vector<std::string> SkelAnimator::allBoneNames()
	{
		std::vector<std::string> names;

		for (int i = 0; i < this->skeleton->num_joints(); ++i) 
		{
			const char* joint_name = skeleton->joint_names()[i];
			names.push_back(joint_name);
		}

		return names;
	}

	void SkelAnimator::setJointWeight(ozz::vector<ozz::math::SimdFloat4>& weights, int joint_index, float value)
	{
		ozz::math::SimdFloat4& soa = weights[joint_index / 4];
		soa = ozz::math::SetI(soa, ozz::math::simd_float4::Load1(value), joint_index % 4);
	}

	

}