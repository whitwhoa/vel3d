
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"

#include "vel/SkelAnimator.h"

namespace vel
{
	SkelAnimator::SkelAnimator(ozz::animation::Skeleton* skeleton) :
		skeleton(skeleton),
		simPrevLocalTransforms(nullptr),
		simLocalTransforms(nullptr)
	{
		
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

	void SkelAnimator::update(float logicTick)
	{
		std::swap(simPrevLocalTransforms, simLocalTransforms);
		this->onUpdate(logicTick);
	}

	void SkelAnimator::renderLerp(float alpha)
	{
		const int n = this->simPrevLocalTransforms->size();
		this->renderLocalTransforms.resize(n);

		const ozz::math::SimdFloat4 t = ozz::math::simd_float4::Load1(alpha);

		for (int i = 0; i < n; ++i)
		{
			const ozz::math::SoaTransform& A = this->simPrevLocalTransforms->at(i);
			const ozz::math::SoaTransform& B = this->simLocalTransforms->at(i);
			ozz::math::SoaTransform& O = this->renderLocalTransforms.at(i);

			O.translation = this->lerpSoaFloat3(A.translation, B.translation, t);
			O.scale = this->lerpSoaFloat3(A.scale, B.scale, t);
			O.rotation = this->nLerpSoaQuaternion(A.rotation, B.rotation, t);
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

}