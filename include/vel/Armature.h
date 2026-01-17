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




namespace vel
{
	class Scene;

	struct TRS
	{
		glm::vec3		translation;
		glm::quat		rotation;
		glm::vec3		scale;
	};

	class Armature
	{
	private:
		std::string											name;
		bool												shouldInterpolate;
		std::vector<ArmatureBone>							bones;
		std::vector<std::shared_ptr<Animation>>				animations;
		std::deque<ActiveAnimation>							activeAnimations;
		float												runTime;
		float												previousRunTime;

		void												updateBone(size_t index, glm::mat4 parentMatrix);
		glm::vec3											calcTranslation(const float& time, size_t currentKeyIndex, Channel* channel);
		glm::quat											calcRotation(const float& time, size_t currentKeyIndex, Channel* channel);
		glm::vec3											calcScale(const float& time, size_t currentKeyIndex, Channel* channel);
		
		Transform											transform;


	public:
		Armature(std::string name);
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
		void												updateAnimation(float runTime);
		std::shared_ptr<Animation>							getAnimation(const std::string& animationName);

		std::string											getCurrentAnimationName();
		unsigned int										getCurrentAnimationCycle();
		float												getCurrentAnimationKeyTime();

		void												playAnimation(const std::string& animationName, bool repeat = true, int blendTime = 0);

		Transform&											getTransform();

	};
}