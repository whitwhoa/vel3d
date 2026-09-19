#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Scene/Scene.h>
#include <vel/Scene/Actor/Actor.h>

namespace vel
{
	Actor::Actor() :
		flags(ACTFLG_NONE),
		lastTransformUpdateTick(0),
		transform(Transform()),
		previousTransform(Transform()),
		parentActor(),
		parentActorBone(-1),
		animator(nullptr),
		mesh(nullptr),
		material(-1),
		stage(nullptr),
		lightmapTexture(-1),
		colorMultiplier({1.f, 1.f, 1.f, 1.f})
	{}

	//Actor::Actor(const Actor& a) :
	//	id(Runtime::_nextId++),
	//	visible(a.visible),
	//	dynamic(a.dynamic),
	//	lerpable(a.lerpable),
	//	lastTransformUpdateTick(0),
	//	transform(a.getTransform()),
	//	previousTransform(a.getPreviousTransform()),
	//	parentActor(std::nullopt),
	//	parentActorBone(-1),
	//	animator(nullptr),
	//	mesh(a.mesh),
	//	material(a.material),
	//	stage(nullptr)
	//{}

	//// TODO: Need to identify why this was done and why it only sets the subset of members
	//Actor& Actor::operator=(const Actor& a)
	//{
	//	if (this == &a)
	//		return *this; // handle self-assignment
	//	
	//	this->id = Runtime::_nextId++;
	//	this->visible = a.visible;
	//	this->dynamic = a.dynamic;
	//	this->lerpable = a.lerpable;
	//	this->transform = a.getTransform();
	//	this->mesh = a.mesh;
	//	this->material = a.material;
	//	this->stage = a.stage;

	//	return *this;
	//}

	void Actor::_updatePrevTransform()
	{
		if (!(this->flags & ACTFLG_DYNAMIC) || ((this->flags & ACTFLG_DYNAMIC) && !(this->flags & ACTFLG_LERPABLE)))
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

	const Transform& Actor::getTransform() const
	{
		return this->transform;
	}

	const Transform& Actor::getPreviousTransform() const
	{
		return this->previousTransform;
	}

	AABB Actor::getWorldAABB()
	{
		if (this->mesh == nullptr)
			return AABB(glm::vec3(0.0f), glm::vec3(0.0f));

		const std::vector<glm::vec3>& localCorners = this->mesh->aabb.getCorners();

		std::vector<glm::vec3> worldCorners;
		worldCorners.reserve(localCorners.size());

		glm::mat4 worldMatrix = this->getWorldMatrix();

		for (const glm::vec3& corner : localCorners)
			worldCorners.push_back(glm::vec3(worldMatrix * glm::vec4(corner, 1.0f)));

		return AABB(worldCorners);
	}

}