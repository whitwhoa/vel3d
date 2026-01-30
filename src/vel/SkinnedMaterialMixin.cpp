#include "vel/SkinnedMaterialMixin.h"
#include "vel/Actor.h"
#include "vel/GPU.h"

namespace vel
{
	SkinnedMaterialMixin::SkinnedMaterialMixin() {}

	void SkinnedMaterialMixin::updateBones(float alphaTime, GPU* gpu, Actor* a)
	{
		Mesh* mesh = a->getMesh();
		Armature* armature = a->getArmature();
		bool armInterp = armature->getShouldInterpolate();

		std::vector<std::pair<unsigned int, glm::mat4>> boneData;

		glm::mat4 meshBoneTransform;
		for (auto& activeBone : a->getActiveBones())
		{
			if (armInterp)
			{
				meshBoneTransform = armature->getBoneWorldMatrixInterpolated(activeBone.first, alphaTime) * mesh->getBone(activeBone.second).offsetMatrix;
			}
			else
			{
				meshBoneTransform = armature->getBoneWorldMatrix(activeBone.first) * mesh->getBone(activeBone.second).offsetMatrix;
			}

			boneData.push_back(std::pair<unsigned int, glm::mat4>(activeBone.second, meshBoneTransform));
		}

		gpu->updateBonesUBO(boneData);
	}
}