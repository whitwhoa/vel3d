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

		size_t boneIndex = 0;
		std::vector<std::pair<unsigned int, glm::mat4>> boneData;

		glm::mat4 meshBoneTransform;
		for (auto& activeBone : a->getActiveBones())
		{
			if (armInterp)
				meshBoneTransform = armature->getBoneWorldMatrixInterpolated(activeBone.first, alphaTime) * mesh->getBone(boneIndex).offsetMatrix;
			else
				meshBoneTransform = armature->getBoneWorldMatrix(activeBone.first) * mesh->getBone(boneIndex).offsetMatrix;

			boneData.push_back(std::pair<unsigned int, glm::mat4>(activeBone.second, meshBoneTransform));
			boneIndex++;
		}

		gpu->updateBonesUBO(boneData);
	}
}