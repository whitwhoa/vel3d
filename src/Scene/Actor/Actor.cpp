#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Scene/Scene.h>
#include <vel/Scene/Actor/Actor.h>

namespace vel
{
	unsigned int Actor::nextActorId = 1;

	Actor::Actor() :
		id(Actor::nextActorId++),
		visible(true),
		dynamic(false),
		lerpable(false),
		lastTransformUpdateTick(0),
		transform(Transform()),
		previousTransform(Transform()),
		parentActor(std::nullopt),
		parentActorBone(-1),
		animator(nullptr),
		mesh(nullptr),
		material(nullptr),
		stage(nullptr),
		userPointer(nullptr)
	{}

	Actor::Actor(const Actor& a) :
		id(Actor::nextActorId++),
		visible(a.visible),
		dynamic(a.dynamic),
		lerpable(a.lerpable),
		lastTransformUpdateTick(0),
		transform(a.getTransform()),
		previousTransform(a.getPreviousTransform()),
		parentActor(std::nullopt),
		parentActorBone(-1),
		animator(nullptr),
		mesh(a.mesh),
		material(a.material),
		stage(nullptr),
		userPointer(nullptr)
	{}

	// TODO: Need to identify why this was done and why it only sets the subset of members
	Actor& Actor::operator=(const Actor& a)
	{
		if (this == &a)
			return *this; // handle self-assignment
		
		this->id = Actor::nextActorId++;
		this->visible = a.visible;
		this->dynamic = a.dynamic;
		this->lerpable = a.lerpable;
		this->transform = a.getTransform();
		this->mesh = a.mesh;
		this->material = a.material;
		this->stage = a.stage;

		return *this;
	}

	unsigned int Actor::getId() const
	{
		return this->id;
	}

	void Actor::_updatePrevTransform()
	{
		if (!this->dynamic || (this->dynamic && !this->lerpable))
			return;

		if (this->lastTransformUpdateTick != Runtime::_currentSimTick)
		{
			this->previousTransform = this->transform;
			this->lastTransformUpdateTick = Runtime::_currentSimTick;
		}
	}

	void Actor::setTranslation(glm::vec3 t)
	{
		this->_updatePrevTransform();
		this->transform.setTranslation(t);
	}

	void Actor::setRotation(float angle, glm::vec3 axis)
	{
		this->_updatePrevTransform();
		this->transform.setRotation(angle, axis);
	}

	void Actor::setRotation(glm::quat r)
	{
		this->_updatePrevTransform();
		this->transform.setRotation(r);
	}

	void Actor::appendRotation(float angle, glm::vec3 axis)
	{
		this->_updatePrevTransform();
		this->transform.appendRotation(angle, axis);
	}

	void Actor::setScale(glm::vec3 s)
	{
		this->_updatePrevTransform();
		this->transform.setScale(s);
	}

	const glm::vec3& Actor::getTranslation() const
	{
		return this->transform.getTranslation();
	}

	const glm::quat& Actor::getRotation() const
	{
		return this->transform.getRotation();
	}

	const glm::vec3& Actor::getScale() const
	{
		return this->transform.getScale();
	}

	glm::mat4 Actor::getMatrix()
	{
		return this->transform.getMatrix();
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
			ozzFloat4x4ToGlmMat4(this->parentActor->animator->getSimBoneMatrix(this->parentActorBone)) * 
			this->transform.getMatrix();
	}

	glm::mat4 Actor::getWorldRenderMatrix(float alpha)
	{
		// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
		if (!this->dynamic || (this->dynamic && !this->lerpable))
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
			ozzFloat4x4ToGlmMat4(this->parentActor->animator->getRenderBoneMatrix(this->parentActorBone)) *
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

	void Actor::setDynamic(bool dynamic, bool lerpable)
	{
		this->dynamic = dynamic;
		this->lerpable = lerpable;
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

	const Transform& Actor::getTransform() const
	{
		return this->transform;
	}

	const Transform& Actor::getPreviousTransform() const
	{
		return this->previousTransform;
	}

	void Actor::setVisible(bool v)
	{
		this->visible = v;

		for (auto& ca : this->childActors)
			ca->setVisible(v);
	}

	bool Actor::isAnimated() const
	{
		if (this->animator)
			return true;

		return false;
	}

	bool Actor::setAnimator(SkelAnimator* a)
	{
		if (!this->mesh)
		{
			SPDLOG_ERROR("Actor::setAnimator(): Attempting to add animator to actor that does not contain a mesh: {}", this->id);
			return false;
		}

		this->animator = a;

		unsigned int index = 0;
		for (auto& meshBone : this->mesh->bones)
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

	const std::vector<std::pair<unsigned int, unsigned int>>& Actor::getActiveBones() const
	{
		return this->activeBones;
	}

	AABB Actor::getWorldAABB()
	{
		if (this->mesh == nullptr)
			return AABB(glm::vec3(0.0f), glm::vec3(0.0f));

		const std::vector<glm::vec3>& localCorners = this->mesh->getAABB().getCorners();

		std::vector<glm::vec3> worldCorners;
		worldCorners.reserve(localCorners.size());

		glm::mat4 worldMatrix = this->getWorldMatrix();

		for (const glm::vec3& corner : localCorners)
			worldCorners.push_back(glm::vec3(worldMatrix * glm::vec4(corner, 1.0f)));

		return AABB(worldCorners);
	}

}