#include <iostream>

#include "spdlog/spdlog.h"

#include "vel/functions.h"
#include "vel/EmptyMaterial.h"
#include "vel/Actor.h"


namespace vel
{
	//unsigned int Actor::copyCount = 0;

	//unsigned int Actor::getNextCopyCount()
	//{
	//	return ++copyCount;
	//}

	Actor::Actor(const std::string& name) :
		name(name),
		visible(true),
		dynamic(false),
		transform(Transform()),
		previousTransform(Transform()),
		parentActor(nullptr),
		parentActorBone(-1),
		animator(nullptr),
		mesh(nullptr),
		//material(nullptr),
		material(std::make_unique<EmptyMaterial>("EMPTY", nullptr)),
		userPointer(nullptr)
	{}

	Actor::Actor(const Actor& a) : 
		//name(a.getName() + "_" + std::to_string(Actor::getNextCopyCount())),
		name(a.getName()),
		visible(a.isVisible()),
		dynamic(a.isDynamic()),
		transform(a.getTransform()),
		previousTransform(a.getPreviousTransform()),
		parentActor(nullptr),
		parentActorBone(-1),
		animator(nullptr),
		mesh(a.getMesh()),
		material(a.getMaterial()->clone()),
		userPointer(nullptr)
	{}

	// TODO: Need to identify why this was done and why it only sets the subset of members
	Actor& Actor::operator=(const Actor& a)
	{
		if (this == &a)
			return *this; // handle self-assignment

		//this->name = a.getName() + "_" + std::to_string(Actor::getNextCopyCount());
		this->name = a.getName();
		this->visible = a.isVisible();
		this->dynamic = a.isDynamic();
		this->transform = a.getTransform();
		this->mesh = a.getMesh();
		this->material = a.getMaterial()->clone();

		return *this;
	}

	void* Actor::getUserPointer()
	{
		return this->userPointer;
	}

	void Actor::setUserPointer(void* p)
	{
		this->userPointer = p;
	}

	void Actor::setMaterial(Material* m)
	{
		this->material = m->clone();
	}

	Material* Actor::getMaterial()
	{
		return this->material.get();
	}

	Material* Actor::getMaterial() const
	{
		return this->material.get();
	}

	//----------------------------------------------------------------------------------------------
	void Actor::_removeChildActor(Actor* child)
	{
		auto& v = this->childActors;
		auto it = std::find(v.begin(), v.end(), child);
		if (it != v.end())
			v.erase(it);
	}

	void Actor::_removeParentActor()
	{
		this->parentActor = nullptr;
	}

	// called from child, means i no longer want to be parented to another actor, so remove me from it's
	// list of children, then remove it's pointer from me
	void Actor::removeParentActor()
	{
		if (!this->parentActor)
			return;

		this->parentActor->_removeChildActor(this);
		this->_removeParentActor();
	}

	// called from parent, means i don't want the provided actor pointer to be parented to me anymore, so
	// remove it from my list, then remove my pointer from the child
	void Actor::removeChildActor(Actor* child)
	{
		if (!child || child->parentActor != this)
			return;

		this->_removeChildActor(child);
		child->_removeParentActor();
	}
	// --------------------------------------------------------------------------

	void Actor::setName(std::string newName)
	{
		this->name = newName;
	}

	void Actor::setMesh(Mesh* m)
	{
		this->mesh = m;
	}

	Mesh* Actor::getMesh()
	{
		return this->mesh;
	}

	Mesh* Actor::getMesh() const
	{
		return this->mesh;
	}

	glm::mat4 Actor::getWorldMatrix()
	{
		// if this actor has no parent, simply return the matrix of it's transform
		if (this->parentActor == nullptr && this->parentActorBone == -1)
			return this->transform.getMatrix();

		// if this actor is parented to another actor, and not to that actor's bone
		if (this->parentActorBone == -1)
			return this->parentActor->getWorldMatrix() * this->transform.getMatrix();

		// if this actor is parented to the bone of its parent actor
		return this->parentActor->getWorldMatrix() * 
			ozzFloat4x4ToGlmMat4(this->parentActor->getAnimator()->getSimBoneMatrix(this->parentActorBone)) * 
			this->transform.getMatrix();
	}

	glm::mat4 Actor::getWorldRenderMatrix(float alpha)
	{
		// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
		if (!this->isDynamic())
			return this->getWorldMatrix();

		glm::mat4 selfMat = Transform::interpolateTransforms(this->previousTransform, this->transform, alpha);

		// if this actor has no parent, simply return the matrix of it's transform
		if (this->parentActor == nullptr && this->parentActorBone == -1)
			return selfMat;

		// if this actor is parented to another actor
		if (this->parentActorBone == -1)
			return this->parentActor->getWorldRenderMatrix(alpha) * selfMat;

		// if we made it here, we know that this actor is parented to a bone of its parent actor
		return this->parentActor->getWorldRenderMatrix(alpha) *
			ozzFloat4x4ToGlmMat4(this->parentActor->getAnimator()->getRenderBoneMatrix(this->parentActorBone)) *
			selfMat;
	}

	glm::vec3 Actor::getInterpolatedTranslation(float alpha)
	{
		return Transform::interpolateTranslations(this->previousTransform, this->transform, alpha);
	}

	glm::quat Actor::getInterpolatedRotation(float alpha)
	{
		return Transform::interpolateRotations(this->previousTransform, this->transform, alpha);
	}

	glm::vec3 Actor::getInterpolatedScale(float alpha)
	{
		return Transform::interpolateScales(this->previousTransform, this->transform, alpha);
	}

	void Actor::setDynamic(bool dynamic)
	{
		this->dynamic = dynamic;
	}

	const bool Actor::isDynamic() const
	{
		return this->dynamic;
	}

	void Actor::updatePreviousTransform()
	{
		if (this->isDynamic())
			this->previousTransform = this->getTransform();
	}

	void Actor::setParentActor(Actor* a)
	{
		// set the parent relationship
		if(a != nullptr)
		{
			this->parentActor = a;
			a->addChildActor(this);
		}
		// remove the parent relationship
		else
		{
			this->removeParentActor();
		}		
	}

	std::vector<Actor*>& Actor::getChildActors()
	{
		return this->childActors;
	}

	void Actor::setParentActorBone(Actor* a, int boneId)
	{
		// set the parent relationship
		if(boneId != -1)
		{
			this->parentActor = a;
			this->parentActor->childActors.push_back(this);
			this->parentActorBone = boneId;
			
			return;
		}

		// remove the parent relationship
		if(this->parentActorBone != -1)
		{
			auto& childrenOfParent = this->parentActor->getChildActors();
			size_t i = 0;
			for(auto& a : childrenOfParent)
			{
				if (a == this)
					childrenOfParent.erase(childrenOfParent.begin() + i);

				i++;
			}

			this->parentActorBone = -1;
		}
	}

	void Actor::addChildActor(Actor* a)
	{
		this->childActors.push_back(a);
	}

	void Actor::setActiveBones(std::vector<std::pair<unsigned int, unsigned int>> activeBones)
	{
		this->activeBones = activeBones;
	}

	const std::vector<std::pair<unsigned int, unsigned int>>& Actor::getActiveBones() const
	{
		return this->activeBones;
	}

	Transform& Actor::getTransform()
	{
		return this->transform;
	}

	const Transform& Actor::getTransform() const
	{
		return this->transform;
	}

	const Transform& Actor::getPreviousTransform() const
	{
		return this->previousTransform;
	}

	const bool Actor::isVisible() const
	{
		return this->visible;
	}

	void Actor::setVisible(bool v)
	{
		this->visible = v;

		for (auto& ca : this->childActors)
			ca->setVisible(v);
	}

	const std::string Actor::getName() const
	{
		return this->name;
	}

	const bool Actor::isAnimated() const
	{
		if (this->animator)
			return true;

		return false;
	}

	bool Actor::setAnimator(SkelAnimator* a)
	{
		if (!this->mesh)
		{
			SPDLOG_ERROR("Actor::setAnimator(): Attempting to add animator to actor that does not contain a mesh: {}", this->name);
			return false;
		}

		this->animator = a;

		unsigned int index = 0;
		for (auto& meshBone : this->getMesh()->getBones())
		{
			int skelBoneIndex = this->animator->getBoneIndex(meshBone.name);
			if (skelBoneIndex == -1)
			{
				SPDLOG_ERROR("Actor::setAnimator(): Skeleton does not contain bone with name {}.", meshBone.name);
				return false;
			}

			this->activeBones.push_back(std::pair<unsigned int, unsigned int>(skelBoneIndex, index));
			index++;
		}

		return true;
	}

	SkelAnimator* Actor::getAnimator()
	{
		return this->animator;
	}

}