#pragma once

#include <vector>
#include <string>

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/containers/vector.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"

namespace vel
{
	class SkelAnimator
	{
	private:
		float										simTime;
		ozz::vector<ozz::math::SoaTransform>		localTransformsA;
		ozz::vector<ozz::math::SoaTransform>		localTransformsB;
		
		ozz::vector<ozz::math::SoaTransform>*		simPrevLocalTransforms;
		
		ozz::vector<ozz::math::SoaTransform>		renderLocalTransforms;
		ozz::vector<ozz::math::Float4x4>			renderModelMatrices;

		ozz::math::SoaFloat3		lerpSoaFloat3(const ozz::math::SoaFloat3& a, const ozz::math::SoaFloat3& b, const ozz::math::SimdFloat4& t);
		ozz::math::SoaQuaternion	nLerpSoaQuaternion(const ozz::math::SoaQuaternion& a, ozz::math::SoaQuaternion b, const ozz::math::SimdFloat4& t);

	protected:
		const ozz::animation::Skeleton*				skeleton;
		ozz::vector<ozz::math::SoaTransform>*		simLocalTransforms;
		ozz::vector<ozz::math::Float4x4>			simModelMatrices;

		void clearMask(ozz::vector<ozz::math::SimdFloat4>& weights, float value = 0.0f);
		static void setJointWeight(ozz::vector<ozz::math::SimdFloat4>& weights, int joint_index, float value);
		static void multiplyJointLocalRotation(int joint, const ozz::math::SimdQuaternion& delta, ozz::span<ozz::math::SoaTransform> transforms);
		static ozz::math::SimdQuaternion extractQuaternionLane(const ozz::math::SoaQuaternion& q, int lane);

	public:
		SkelAnimator(ozz::animation::Skeleton* skeleton);

		virtual bool	init() = 0;
		virtual bool	onUpdate(float logicTick) = 0;

		void			update(float logicTick);
		void			renderLerp(float alpha);

		float			getSimTime() const;
		const ozz::math::Float4x4& getSimBoneMatrix(unsigned int i);
		const ozz::math::Float4x4& getRenderBoneMatrix(unsigned int i);
		int getBoneIndex(const std::string& name);
		std::vector<std::string> allBoneNames();



		
		

	};
}