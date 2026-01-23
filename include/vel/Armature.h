#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "vel/Animation.h"
#include "vel/Mesh.h"
#include "vel/ArmatureBone.h"
#include "vel/Transform.h"
#include "vel/ActiveAnimation.h"
#include "vel/TRS.h"



namespace vel
{
	class Scene;

	class Armature
	{
	private:
		std::string											name;
		bool												shouldInterpolate;
		std::vector<ArmatureBone>							bones;
		std::vector<std::shared_ptr<Animation>>				animations;
		std::vector<std::deque<ActiveAnimation>>			layers;
		float												runTime;
		float												previousRunTime;
		Transform											transform;

		std::vector<TRS>									poseA;
		std::vector<TRS>									poseB;
		TRS*												posePrev;
		TRS*												poseCur;


		//void												updateBone(size_t layerIndex, size_t boneIndex);
		void												updateBone(size_t boneIndex);

		glm::vec3											calcTranslation(const float& time, size_t currentKeyIndex, Channel* channel);
		glm::quat											calcRotation(const float& time, size_t currentKeyIndex, Channel* channel);
		glm::vec3											calcScale(const float& time, size_t currentKeyIndex, Channel* channel);

		vel::TRS											composeWorldTRS(const vel::TRS& parentW, const vel::TRS& local);
		glm::mat4											matrixFromTRS(const vel::TRS& t);
		
		


	public:
		Armature(std::string name);
		void												initPoseBuffers();
		void												setShouldInterpolate(bool val);
		bool												getShouldInterpolate();
		void												addBone(ArmatureBone b);
		void												addAnimation(std::shared_ptr<Animation> anim);
		ArmatureBone&										getRootBone();
		std::vector<ArmatureBone>&							getBones();
		const std::vector<ArmatureBone>&					getBones() const;
		ArmatureBone&										getBone(unsigned int index);
		ArmatureBone*										getBone(const std::string& boneName);
		const std::string&									getName() const;
		const std::vector<std::shared_ptr<Animation>>&		getAnimations() const;
		std::optional<size_t>								getBoneIndex(const std::string& boneName);
		std::shared_ptr<Animation>							getAnimation(const std::string& animationName);
		void												updateLayer(unsigned int id, float stepTime);
		void												updateAnimations(float runTime);
		

		unsigned int										addAnimationLayer();
		unsigned int										getAnimationLayerCount();

		void												playAnimation(const std::string& animationName, bool repeat = true, int blendTime = 0);
		void												playAnimation(unsigned int layerIndex, const std::string& animationName, bool repeat = true, int blendTime = 0);
		
		//std::string										getCurrentAnimationName();
		//unsigned int										getCurrentAnimationCycle();
		//float												getCurrentAnimationKeyTime();

		
		Transform&											getTransform();

		const glm::mat4&									getBoneWorldMatrix(size_t i);
		glm::mat4											getBoneWorldMatrixInterpolated(size_t i, float alpha);
	};
}