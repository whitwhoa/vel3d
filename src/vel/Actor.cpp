#include <iostream>

#include "vel/functions.h"
#include "vel/Actor.h"


namespace vel
{
	unsigned int Actor::copyCount = 0;

	unsigned int Actor::getNextCopyCount()
	{
		return ++copyCount;
	}

	Actor::Actor(std::string name) :
		name(name),
		visible(true),
		dynamic(false),
		transform(Transform()),
		parentActor(nullptr),
		parentArmatureBone(nullptr),
		armature(nullptr),
		mesh(nullptr),
		material(nullptr),
		userPointer(nullptr)
	{}

	Actor::Actor(const Actor& a) : 
		name(a.getName() + "_" + std::to_string(Actor::getNextCopyCount())),
		visible(a.isVisible()),
		dynamic(a.isDynamic()),
		transform(a.getTransform()),
		parentActor(nullptr),
		parentArmatureBone(nullptr),
		armature(nullptr),
		mesh(a.getMesh()),
		material(std::make_unique<Material>(*a.getMaterial())),
		userPointer(nullptr)
	{}

	Actor& Actor::operator=(const Actor& a)
	{
		if (this == &a)
			return *this; // handle self-assignment

		this->name = a.getName() + "_" + std::to_string(Actor::getNextCopyCount());
		this->visible = a.isVisible();
		this->dynamic = a.isDynamic();
		this->transform = a.getTransform();
		this->mesh = a.getMesh();
		this->material = std::make_unique<Material>(*a.getMaterial());

		return *this;
	}

	std::vector<glm::vec3>& Actor::getGIColors()
	{
		return this->giColors;
	}

	void Actor::updateGIColors(std::vector<glm::vec3>& colors)
	{
		this->giColors = colors;
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
		this->material = std::make_unique<Material>(*m);
	}

	Material* Actor::getMaterial()
	{
		return this->material.get();
	}

	Material* Actor::getMaterial() const
	{
		return this->material.get();
	}

	void Actor::clearPreviousTransform()
	{
		this->previousTransform.reset();
	}

	void Actor::removeChildActor(Actor* aIn, bool calledFromRemoveParentActor)
	{
		for (size_t i = 0; i < this->childActors.size(); i++)
		{
			if (this->childActors.at(i) == aIn)
			{
				if (!calledFromRemoveParentActor)
					aIn->removeParentActor(true);

				this->childActors.erase(this->childActors.begin() + i);
			}
		}
	}

	void Actor::removeParentActor(bool calledFromRemoveChildActor)
	{
		if (this->parentActor != nullptr)
		{
			if (!calledFromRemoveChildActor)
				this->parentActor->removeChildActor(this, true);

			this->parentActor = nullptr;
		}
	}

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
		if (this->parentActor == nullptr && this->parentArmatureBone == nullptr)
			return this->transform.getMatrix();

		// if this actor is parented to another actor (parenting to actor will override bone parenting)
		if (this->parentArmatureBone == nullptr)
			return this->parentActor->getWorldMatrix() * this->transform.getMatrix();

		//return this->parentActor->getWorldMatrix() * this->parentArmatureBone->matrix * this->transform.getMatrix();
		return this->parentArmatureBone->matrix * this->transform.getMatrix();
	}

	glm::mat4 Actor::getWorldRenderMatrix(float alpha)
	{
		// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
		// TODO: but what if at some point it is parented to a dynamic actor? Well, the easiest solution would just be
		// to assume that only dynamic actors can ever be parented to other dynamic actors...which in a way makes sense
		// as really any actor that is not dynamic would be objects such as non-interactive map geometry. Moving forward
		// with this approach (at least for the time being)
		if (!this->isDynamic() || !this->previousTransform)
			return this->getWorldMatrix();

		auto actorMatrix = Transform::interpolateTransforms(this->previousTransform.value(), this->transform, alpha);

		// if this actor has no parent, simply return the matrix of it's transform
		if (this->parentActor == nullptr && this->parentArmatureBone == nullptr)
			return actorMatrix;

		// if this actor is parented to another actor (parenting to actor will override bone parenting)
		if (this->parentArmatureBone == nullptr)
			return this->parentActor->getWorldRenderMatrix(alpha) * actorMatrix;

		//auto boneMatrix = this->parentArmatureBone->getRenderMatrix(alpha);
		//return this->parentActor->getWorldRenderMatrix(alpha) * boneMatrix * actorMatrix;
		
		if (this->parentArmatureBone->parentArmature->getShouldInterpolate())
			return this->parentArmatureBone->getRenderMatrixInterpolated(alpha) * actorMatrix;
		
		return this->parentArmatureBone->getRenderMatrix() * actorMatrix;
	}

	glm::vec3 Actor::getInterpolatedTranslation(float alpha)
	{
		if (this->previousTransform) // insure we have a value for previousTransform from which to interpolate
			return Transform::interpolateTranslations(this->previousTransform.value(), this->transform, alpha);

		return this->transform.getTranslation();
	}

	glm::quat Actor::getInterpolatedRotation(float alpha)
	{
		if (this->previousTransform) // insure we have a value for previousTransform from which to interpolate
			return Transform::interpolateRotations(this->previousTransform.value(), this->transform, alpha);

		return this->transform.getRotation();
	}

	glm::vec3 Actor::getInterpolatedScale(float alpha)
	{
		if (this->previousTransform) // insure we have a value for previousTransform from which to interpolate
			return Transform::interpolateScales(this->previousTransform.value(), this->transform, alpha);

		return this->transform.getScale();
	}

	void Actor::setEmptyGIColors()
	{
		this->giColors = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) };
	}

	void Actor::setDynamic(bool dynamic)
	{
		this->dynamic = dynamic;
		// TODO: do we want to be setting these here, what if we want to go from dynamic to none dynamic and retain current giColor...????
		if (dynamic)
			this->setEmptyGIColors();
		else
			this->giColors.clear();
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

	void Actor::setParentArmatureBone(ArmatureBone* b)
	{
		// set the parent relationship
		if(b != nullptr)
		{
			this->parentArmatureBone = b;
			b->childActors.push_back(this);
		}
		// remove the parent relationship
		else if(this->parentArmatureBone != nullptr)
		{
			size_t i = 0;
			for(auto& a : this->parentArmatureBone->childActors)
			{
				if(a == this)
					this->parentArmatureBone->childActors.erase(this->parentArmatureBone->childActors.begin() + i);
				
				i++;
			}
			this->parentArmatureBone = nullptr;
		}
	}

	void Actor::addChildActor(Actor* a)
	{
		this->childActors.push_back(a);
	}

	void Actor::setActiveBones(std::vector<std::pair<size_t, unsigned int>> activeBones)
	{
		this->activeBones = activeBones;
	}

	const std::vector<std::pair<size_t, unsigned int>>& Actor::getActiveBones() const
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

	std::optional<Transform>& Actor::getPreviousTransform()
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
		if (this->armature)
			return true;

		return false;
	}

	void Actor::setArmature(Armature* arm)
	{
		this->armature = arm;
	}

	Armature* Actor::getArmature()
	{
		return this->armature;
	}

	Armature* Actor::getArmature() const
	{
		return this->armature;
	}

}