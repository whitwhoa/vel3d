#include <iostream>
#include <algorithm>


#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/matrix_transform.hpp"
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
		// default to a single animation layer
		this->addAnimationLayer();
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

		//return glm::normalize(glm::slerp(channel->rotationKeyValues[currentKeyIndex], channel->rotationKeyValues[nextKeyIndex], factor));
		return glm::slerp(channel->rotationKeyValues[currentKeyIndex], channel->rotationKeyValues[nextKeyIndex], factor);
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

	void Armature::updateBone(size_t layerIndex, size_t boneIndex)
	{
		//
		// !!!! we assume there are always at least 2 keys of animation data !!!!
		//

		
		// update previous translation/rotation/scale (required for timestep interpolation)
		ArmatureBone& bone = this->bones[boneIndex];
		bone.previousTranslation = bone.translation;
		bone.previousRotation = bone.rotation;
		bone.previousScale = bone.scale;


		TRS localTRS{};
		bool haveLocal = false;

		for(unsigned int i = 0; i < this->layers.at(layerIndex).size(); i++)
		{
			ActiveAnimation& aa = this->layers.at(layerIndex).at(i);

			if (aa.animation->channelMask.test(boneIndex))
			{
				// bone exists in mask, do not include transforms for this bone for this animation
				// in the final calculation
				continue;
			}				

			Channel* channel = &aa.animation->channels[boneIndex];
			auto it = std::upper_bound(channel->positionKeyTimes.begin(), channel->positionKeyTimes.end(), aa.animationKeyTime);
			size_t tmpKey = (size_t)(it - channel->positionKeyTimes.begin());
			size_t currentKeyIndex = !(tmpKey == channel->positionKeyTimes.size()) ? (tmpKey - 1) : (tmpKey - 2);

			TRS aaTRS;
			aaTRS.translation = this->calcTranslation(aa.animationKeyTime, currentKeyIndex, channel);
			aaTRS.rotation = this->calcRotation(aa.animationKeyTime, currentKeyIndex, channel);
			aaTRS.scale = this->calcScale(aa.animationKeyTime, currentKeyIndex, channel);

			if (!haveLocal)
			{
				// first (or only) animation in the queue, no blending required
				localTRS = aaTRS;

				// set haveLocal to true, so that we will know later outside of this loop that at
				// least one animation in the layer was not masked and that we need to include
				// the transforms of this bone in the final calculation
				haveLocal = true;
				
				continue;
			}
			
			// blend this animation on top of previous result
			localTRS.translation = glm::lerp(localTRS.translation, aaTRS.translation, aa.blendPercentage);
			localTRS.rotation = glm::slerp(localTRS.rotation, aaTRS.rotation, aa.blendPercentage);
			localTRS.scale = glm::lerp(localTRS.scale, aaTRS.scale, aa.blendPercentage);
		}

		// this bone did not contribute to the animation on this layer, proceed no further
		if (!haveLocal)
			return;

		// we store the bone positions in world space for easy attachments and timestep interpolation
		//
		// first bone is always armature root bone. This is what we translate/rotate/scale to move
		// the armature around the stage, which in turn moves all associated actors, thus, we set the
		// transform of this bone to the position of the transform member we hold on Armature
		TRS parentWorld;
		if (boneIndex == 0)
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

	void Armature::updateAnimations(float runTime)
	{
		this->previousRunTime = this->runTime;
		this->runTime = runTime;
		auto stepTime = this->runTime - this->previousRunTime;

		// don't like having to do this at all, but perhaps once we see everything working,
		// we can go back and refactor all of this logic so that we only loop over all bones once somehow
		for (unsigned int i = 0; i < this->bones.size(); i++)
		{
			auto& b = this->bones[i];
			b.previousTranslation = b.translation;
			b.previousRotation = b.rotation;
			b.previousScale = b.previousScale;
		}

		for (unsigned int i = 0; i < this->layers.size(); i++)
			this->updateLayer(i, stepTime);
	}

	void Armature::updateLayer(unsigned int id, float stepTime)
	{
		//
		// Updates various required information for each ActiveAnimation of the layer belonging to `id`,
		// then updates each armature bone by calling updateBone() method which uses the previously
		// updated ActiveAnimations data
		//

		// get most recent layer animation (ie what bones are blending toward)
		auto& layerAnimation = this->layers.at(id).back();

		// current layer animation has completed, and is not set to repeat
		if (!(layerAnimation.repeat || (!layerAnimation.repeat && layerAnimation.currentAnimationCycle == 0)))
			return;

		
		layerAnimation.blendPercentage = 1.0f;

		if (layerAnimation.blendTime > 0.0f)
			layerAnimation.blendPercentage = (layerAnimation.animationTime / (layerAnimation.blendTime / 1000.0f));


		// if blendPercentage is >= 1.0f, we have completed the blending phase and this animation can continue to play without 
		// interpolating between previous animations, therefore we clear all previous animations from this->layers.at(id)
		if (layerAnimation.blendPercentage >= 1.0f)
		{
			for (size_t i = 0; i < this->layers.at(id).size() - 1; i++)
				this->layers.at(id).pop_front();

			layerAnimation = this->layers.at(id).back();
		}


		layerAnimation.lastAnimationKeyTime = layerAnimation.animationKeyTime;
		layerAnimation.animationKeyTime = fmod(layerAnimation.animationTime * layerAnimation.animation->tps, layerAnimation.animation->duration);

		// if true, animation has wrapped and we need to increment the animation cycle count
		if (layerAnimation.animationKeyTime < layerAnimation.lastAnimationKeyTime)
			layerAnimation.currentAnimationCycle++;


		// if we have JUST completed an animation cycle THIS tick, and the animation is not set to repeat
		if (layerAnimation.currentAnimationCycle == 1 && !layerAnimation.repeat)
		{
			layerAnimation.animationKeyTime = layerAnimation.lastAnimationKeyTime;

			for (size_t i = 0; i < this->bones.size(); i++)
			{
				auto& bone = this->bones[i];
				bone.previousTranslation = bone.translation;
				bone.previousRotation = bone.rotation;
				bone.previousScale = bone.scale;
			}

			return;
		}

		// update all armature bones and increase animation time for this animation
		for (size_t i = 0; i < this->bones.size(); i++)
			this->updateBone(id, i);

		layerAnimation.animationTime += stepTime;
	}

	void Armature::playAnimation(const std::string& animationName, bool repeat, int blendTime)
	{
		this->queueLayerAnimation(0, animationName, repeat, blendTime);
	}

	unsigned int Armature::addAnimationLayer()
	{
		this->layers.push_back(std::deque<ActiveAnimation>());
		return this->layers.size();
	}

	unsigned int Armature::getAnimationLayerCount()
	{
		return this->layers.size();
	}

	void Armature::queueLayerAnimation(unsigned int layerIndex, const std::string& animationName, bool repeat, int blendTime)
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

		this->layers.at(layerIndex).push_back(a);
	}

	//float Armature::getCurrentAnimationKeyTime()
	//{
	//	return this->activeAnimations.back().animationKeyTime;
	//}

	//unsigned int Armature::getCurrentAnimationCycle()
	//{
	//	if (this->activeAnimations.size() > 0)
	//		return this->activeAnimations.back().currentAnimationCycle;
	//	else
	//		return 0;
	//}

	//std::string Armature::getCurrentAnimationName()
	//{
	//	if (this->activeAnimations.size() > 0)
	//		return this->activeAnimations.back().animationName;
	//	else
	//		return "";
	//}

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