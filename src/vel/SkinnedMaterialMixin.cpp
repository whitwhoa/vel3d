#include "vel/SkinnedMaterialMixin.h"
#include "vel/Actor.h"
#include "vel/GPU.h"
#include "vel/functions.h"

namespace vel
{
	SkinnedMaterialMixin::SkinnedMaterialMixin() {}

	void SkinnedMaterialMixin::updateBones(float alphaTime, GPU* gpu, Actor* a)
	{
		Mesh* mesh = a->getMesh();
		SkelAnimator* animator = a->getAnimator();

		std::vector<std::pair<unsigned int, glm::mat4>> boneData;

		glm::mat4 meshBoneTransform;
		for (auto& activeBone : a->getActiveBones())
		{		
			meshBoneTransform = ozzFloat4x4ToGlmMat4(animator->getRenderBoneMatrix(activeBone.first)) * mesh->getBone(activeBone.second).offsetMatrix;
			boneData.push_back(std::pair<unsigned int, glm::mat4>(activeBone.second, meshBoneTransform));
		}

		gpu->updateBonesUBO(boneData);
	}
}