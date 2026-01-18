#include <iostream>
#include <algorithm>


//#define GLM_FORCE_ALIGNED_GENTYPES
#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/string_cast.hpp"

#include "vel/logger.hpp"
#include "vel/Armature.h"


namespace vel
{
	Armature::Armature(std::string name) :
		name(name),
		shouldInterpolate(true),
		runTime(0.0),
		previousRunTime(0.0),
		transform(Transform())
	{
	}

	void Armature::setShouldInterpolate(bool val)
	{
		this->shouldInterpolate = val;
	}

	bool Armature::getShouldInterpolate()
	{
		return this->shouldInterpolate;
	}

	Transform& Armature::getTransform()
	{
		return this->transform;
	}

	glm::vec3 Armature::calcTranslation(const float& time, size_t currentKeyIndex, Channel* channel)
	{
		size_t nextKeyIndex = currentKeyIndex + 1;

		float deltaTime = channel->positionKeyTimes[nextKeyIndex] - channel->positionKeyTimes[currentKeyIndex];
		float factor = ((time - channel->positionKeyTimes[currentKeyIndex]) / deltaTime);

		return channel->positionKeyValues[currentKeyIndex] + factor * (channel->positionKeyValues[nextKeyIndex] - channel->positionKeyValues[currentKeyIndex]);
	}

	glm::quat Armature::calcRotation(const float& time, size_t currentKeyIndex, Channel* channel)
	{
		size_t nextKeyIndex = currentKeyIndex + 1;

		float deltaTime = channel->rotationKeyTimes[nextKeyIndex] - channel->rotationKeyTimes[currentKeyIndex];
		float factor = ((time - channel->rotationKeyTimes[currentKeyIndex]) / deltaTime);

		return glm::normalize(glm::slerp(channel->rotationKeyValues[currentKeyIndex], channel->rotationKeyValues[nextKeyIndex], factor));
	}

	glm::vec3 Armature::calcScale(const float& time, size_t currentKeyIndex, Channel* channel)
	{
		size_t nextKeyIndex = currentKeyIndex + 1;

		float deltaTime = channel->scalingKeyTimes[nextKeyIndex] - channel->scalingKeyTimes[currentKeyIndex];
		float factor = ((time - channel->scalingKeyTimes[currentKeyIndex]) / deltaTime);

		return channel->scalingKeyValues[currentKeyIndex] + factor * (channel->scalingKeyValues[nextKeyIndex] - channel->scalingKeyValues[currentKeyIndex]);
	}

	vel::TRS Armature::composeWorldTRS(const vel::TRS& parentW, const vel::TRS& local)
	{
		vel::TRS out;

		// Rotation: parent then local
		out.rotation = parentW.rotation * local.rotation;

		// Scale: component-wise (assumes no shear)
		out.scale = parentW.scale * local.scale;

		// Translation: Tp + Rp * (Sp * Tl)
		out.translation = parentW.translation + (parentW.rotation * (parentW.scale * local.translation));

		return out;
	}

	glm::mat4 Armature::matrixFromTRS(const vel::TRS& t)
	{
		glm::mat4 m(1.0f);
		m = glm::translate(m, t.translation);
		m = m * glm::toMat4(t.rotation);
		m = glm::scale(m, t.scale);
		return m;
	}

	void Armature::updateBone(size_t index)
	{
		// we assume there are always at least 2 keys of animation data

		// get current bone and update its previous WORLD TRS values with current
		auto& bone = this->bones[index];
		bone.previousTranslation = bone.translation;
		bone.previousRotation = bone.rotation;
		bone.previousScale = bone.scale;

		//VEL3D_LOG_TRACE("{}", bone.name);

		//// DEBUG: quick dirty test
		//if (bone.name == "Bip01_Spine1" || bone.name == "")
		//{
		//	TRS worldTRS;
		//	worldTRS.translation = bone.translation;
		//	worldTRS.rotation = bone.rotation;
		//	worldTRS.scale = bone.scale;
		//	bone.matrix = this->matrixFromTRS(worldTRS);

		//	return;
		//}
		//// END

		// get vector of key indexes where vector index is the index of the activeAnimation and value is the keyIndex
		std::vector<TRS> activeAnimationsTRS;
		activeAnimationsTRS.reserve(this->activeAnimations.size());

		for (auto& aa : this->activeAnimations)
		{
			TRS trs;

			// TODO: commented below out, because I'm going to commit this and don't want this check to be done until
			// we can implement the real solution. We will pick back up here, implement this, then move onto a solution
			// that allows us to remove all of the string hash lookups for: auto channel = &aa.animation->channels[bone.name];
			//// DEBUG: quick dirty test
			//if (bone.name == "Bip01_Spine1" || bone.name == "Bip01_R_Clavicle" || bone.name == "Bip01_L_Clavicle"
			//	|| bone.name == "Bip01_Neck" || bone.name == "Bip01_Head" || bone.name == "Bip01_L_UpperArm"
			//	|| bone.name == "Bip01_L_Forearm" || bone.name == "Bip01_L_ForeTwist" || bone.name == "Bip01_L_Hand" 
			//	|| bone.name == "Bip01_L_Finger0" || bone.name == "Bip01_L_Finger01" || bone.name == "Bip01_L_Finger1" 
			//	|| bone.name == "Bip01_L_Finger11" || bone.name == "Bip01_L_Finger2" || bone.name == "Bip01_L_Finger21" 
			//	|| bone.name == "Bip01_R_UpperArm" || bone.name == "Bip01_R_Forearm" || bone.name == "Bip01_R_ForeTwist" 
			//	|| bone.name == "Bip01_R_Hand" || bone.name == "Bip01_R_Finger0" || bone.name == "Bip01_R_Finger01" 
			//	|| bone.name == "Bip01_R_Finger1" || bone.name == "Bip01_R_Finger11" || bone.name == "Bip01_R_Finger2" 
			//	|| bone.name == "Bip01_R_Finger21" || bone.name == "weapon_Bone" || bone.name == "knife_Bone")
			//{
			//	trs.translation = bone.restLocalTranslation;
			//	trs.rotation = bone.restLocalRotation;
			//	trs.scale = bone.restLocalScale;

			//	activeAnimationsTRS.push_back(trs);

			//	continue;
			//}
			//// END

			auto channel = &aa.animation->channels[bone.name];
			auto it = std::upper_bound(channel->positionKeyTimes.begin(), channel->positionKeyTimes.end(), aa.animationKeyTime);
			size_t tmpKey = (size_t)(it - channel->positionKeyTimes.begin());
			size_t currentKeyIndex = !(tmpKey == channel->positionKeyTimes.size()) ? (tmpKey - 1) : (tmpKey - 2);

			trs.translation = this->calcTranslation(aa.animationKeyTime, currentKeyIndex, channel);
			trs.rotation = this->calcRotation(aa.animationKeyTime, currentKeyIndex, channel);
			trs.scale = this->calcScale(aa.animationKeyTime, currentKeyIndex, channel);

			activeAnimationsTRS.push_back(trs);
		}


		//
		// Compose LOCAL TRS
		//
		TRS localTRS;

		// if activeAnimations has a size greater than 1, then do interpolation
		if (activeAnimationsTRS.size() > 1)
		{
			for (size_t i = 0; i < activeAnimationsTRS.size() - 1; i++)
			{
				// weight is stored on the "newer" animation layer
				const float w = this->activeAnimations[i + 1].blendPercentage;

				if (i == 0)
				{
					localTRS.translation = glm::lerp(activeAnimationsTRS[i].translation, activeAnimationsTRS[i + 1].translation, w);
					localTRS.rotation = glm::slerp(activeAnimationsTRS[i].rotation, activeAnimationsTRS[i + 1].rotation, w);
					localTRS.scale = glm::lerp(activeAnimationsTRS[i].scale, activeAnimationsTRS[i + 1].scale, w);
				}
				else
				{
					localTRS.translation = glm::lerp(localTRS.translation, activeAnimationsTRS[i + 1].translation, w);
					localTRS.rotation = glm::slerp(localTRS.rotation, activeAnimationsTRS[i + 1].rotation, w);
					localTRS.scale = glm::lerp(localTRS.scale, activeAnimationsTRS[i + 1].scale, w);
				}
			}
		}
		// otherwise, generate the bone matrix using the single animation
		else
		{
			localTRS = activeAnimationsTRS[0];
		}


		//
		// Compose WORLD TRS (easy attachments and interpolation)
		//
		TRS parentWorld;

		if (index == 0)
		{
			parentWorld.translation = this->transform.getTranslation();
			parentWorld.rotation = this->transform.getRotation();
			parentWorld.scale = this->transform.getScale();
		}
		else
		{
			const auto& p = this->bones[bone.parent];
			parentWorld.translation = p.translation;
			parentWorld.rotation = p.rotation;
			parentWorld.scale = p.scale;
		}

		const TRS worldTRS = this->composeWorldTRS(parentWorld, localTRS);
		bone.translation = worldTRS.translation;
		bone.rotation = worldTRS.rotation;
		bone.scale = worldTRS.scale;
		bone.matrix = this->matrixFromTRS(worldTRS);
	}

	void Armature::updateAnimation(float runTime)
	{
		this->previousRunTime = this->runTime;
		this->runTime = runTime;
		auto stepTime = this->runTime - this->previousRunTime;

		// get most recent active animation
		auto& activeAnimation = this->activeAnimations.back();

		// current active animation is either set to repeat, or not repeat but has not completed it's first cycle
		if (activeAnimation.repeat || (!activeAnimation.repeat && activeAnimation.currentAnimationCycle == 0))
		{

			if (activeAnimation.blendTime > 0.0)
			{
				activeAnimation.blendPercentage = (float)(activeAnimation.animationTime / (activeAnimation.blendTime / 1000.0));
			}
			else
			{
				activeAnimation.blendPercentage = 1.0f;
			}
				

			// if blendPercentage is greater than or equal to 1.0f, then we have completed the blending phase and this animation
			// can continue to play without interpolating between previous animations, therefore we clear all previous animations
			// from this->activeAnimations
			if (activeAnimation.blendPercentage >= 1.0f)
			{
				for (size_t i = 0; i < this->activeAnimations.size() - 1; i++)
					this->activeAnimations.pop_front();

				activeAnimation = this->activeAnimations.back();
			}

			activeAnimation.lastAnimationKeyTime = activeAnimation.animationKeyTime;
			activeAnimation.animationKeyTime = (float)fmod(activeAnimation.animationTime * activeAnimation.animation->tps, activeAnimation.animation->duration);

			if (activeAnimation.animationKeyTime < activeAnimation.lastAnimationKeyTime)
				activeAnimation.currentAnimationCycle++;

			if (activeAnimation.currentAnimationCycle == 1 && !activeAnimation.repeat)
			{
				activeAnimation.animationKeyTime = activeAnimation.lastAnimationKeyTime;

				for (size_t i = 0; i < this->bones.size(); i++)
				{
					auto& bone = this->bones[i];
					bone.previousTranslation = bone.translation;
					bone.previousRotation = bone.rotation;
					bone.previousScale = bone.scale;
				}
			}
			else
			{
				for (size_t i = 0; i < this->bones.size(); i++)
				{
					this->updateBone(i);
				}

				activeAnimation.animationTime += stepTime;
			}
		}
	}

	void Armature::setRestPose(const std::string& animationName)
	{
		std::shared_ptr<vel::Animation> a = this->getAnimation(animationName);
		
		for (auto& b : this->bones)
		{
			b.restLocalTranslation = a->channels[b.name].positionKeyValues.at(0);
			b.restLocalRotation = a->channels[b.name].rotationKeyValues.at(0);
			b.restLocalScale = a->channels[b.name].scalingKeyValues.at(0);
		}
	}

	void Armature::playAnimation(const std::string& animationName, bool repeat, int blendTime)
	{
		ActiveAnimation a;
		a.animation = this->getAnimation(animationName);
		a.animationName = animationName;
		a.blendTime = (float)blendTime;
		a.animationTime = 0.0;
		a.animationKeyTime = 0.0f;
		a.lastAnimationKeyTime = 0.0f;
		a.currentAnimationCycle = 0;
		a.blendPercentage = 0.0f;
		a.repeat = repeat;


		this->activeAnimations.push_back(a);
	}

	float Armature::getCurrentAnimationKeyTime()
	{
		return this->activeAnimations.back().animationKeyTime;
	}

	unsigned int Armature::getCurrentAnimationCycle()
	{
		if (this->activeAnimations.size() > 0)
			return this->activeAnimations.back().currentAnimationCycle;
		else
			return 0;
	}

	std::string Armature::getCurrentAnimationName()
	{
		if (this->activeAnimations.size() > 0)
			return this->activeAnimations.back().animationName;
		else
			return "";
	}

	const std::vector<std::shared_ptr<Animation>>& Armature::getAnimations() const
	{
		return this->animations;
	}

	const std::string& Armature::getName() const
	{
		return this->name;
	}

	void Armature::addAnimation(std::shared_ptr<Animation> anim)
	{
		this->animations.push_back(anim);
	}

	void Armature::addBone(ArmatureBone b)
	{
		this->bones.push_back(b);
	}

	ArmatureBone& Armature::getRootBone()
	{
		return this->bones[0];
	}

	std::vector<ArmatureBone>& Armature::getBones()
	{
		return this->bones;
	}

	const std::vector<ArmatureBone>& Armature::getBones() const
	{
		return this->bones;
	}

	ArmatureBone* Armature::getBone(const std::string& boneName)
	{
		for (auto& b : this->bones)
			if (b.name == boneName)
				return &b;

		return nullptr;
	}

	ArmatureBone& Armature::getBone(unsigned int index)
	{
		return this->bones.at(index);
	}

	std::shared_ptr<Animation> Armature::getAnimation(const std::string& animationName)
	{
		for (auto& p : this->animations)
		{
			if (p->name == animationName)
				return p;
		}

		VEL3D_LOG_DEBUG("Armature::getAnimation(): Attempting to get animation pointer of non-existing animation name: {}", animationName);
		return nullptr;
	}

	std::optional<size_t> Armature::getBoneIndex(const std::string& boneName)
	{
		for (size_t i = 0; i < this->bones.size(); i++)
			if (this->bones.at(i).name == boneName)
				return i;

		VEL3D_LOG_DEBUG("Armature::getBoneIndex(): Attempting to get index of non-existing bone name: {}", boneName);
		return std::nullopt;
	}

}