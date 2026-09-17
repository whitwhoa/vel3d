#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <vel/Scene/Animation/SkelAnimator.h>
#include <vel/Scene/Mesh/Mesh.h>
#include <vel/Scene/Actor/Transform.h>
#include <vel/Scene/Material.h>
#include <vel/Scene/Stage/Stage.h>

#include <vel/Util/slot_map.h>



namespace vel
{
	typedef slot_handle actor_handle;

	enum ActFlg : uint32_t
	{
		ACTFLG_NONE = 0,
		ACTFLG_VISIBLE = 1 << 0,
		ACTFLG_DYNAMIC = 1 << 1,
		ACTFLG_LERPABLE = 1 << 2,
		ACTFLG_BILLBOARD = 1 << 3,
		ACTFLG_BILLBOARD_LOCK_Y = 1 << 4,
		ACTFLG_ANIMATED_MATERIAL = 1 << 5
	};

	class Actor
	{
	private:
		Transform						transform;
		Transform						previousTransform;
		std::vector<std::pair<uint32_t, uint32_t>> activeBones; // the bones from the armature used by the mesh, .first = animator.renderModelMatrices index, .second = mesh.bones index
	public:
		std::vector<actor_handle>		childActors; // If size() > 0, this actor is a parent to all actors referenced by contained slot_handles
	private:
		uint32_t						lastTransformUpdateTick;
	public:
		int32_t							parentActorBone; // Used in conjunction with parentActor, when present (> -1)
		uint32_t						flags;
		material_handle					material;
		actor_handle					parentActor; // If has value, this actor is a child of the actor referenced by slot_handle
		SkelAnimator*					animator;
		Mesh*							mesh;
		Stage*							stage;

	private:
		void						_updatePrevTransform();
		
	public:
		Actor();
		//Actor(const Actor& original);
		//Actor& operator=(const Actor& a);

		// TODO: move to HeadlessScene
		bool						setAnimator(SkelAnimator* a); // !!! LEFT OFF HERE !!!!!
		void						setParentActor(Actor* a);
		void						setParentActorBone(Actor* a, int boneId);
		void						addChildActor(Actor* a);
		void						removeParentActor();
		void						removeChildActor(Actor* child);
		void						_removeParentActor();
		void						_removeChildActor(Actor* child);
		// END


		void						setTranslation(glm::vec3 t);
		void						setRotation(float angle, glm::vec3 axis);
		void						setRotation(glm::quat r);
		void						appendRotation(float angle, glm::vec3 axis);
		void						setScale(glm::vec3 s);

		bool						isAnimated() const;
		const std::vector<std::pair<unsigned int, unsigned int>>& getActiveBones() const;
		const Transform&			getTransform() const;
		const Transform&			getPreviousTransform() const;
		glm::mat4					getWorldMatrix();
		glm::mat4					getWorldRenderMatrix(float alpha); // contains logic for interpolation
		glm::vec3					getInterpolatedTranslation(float alpha);
		glm::quat					getInterpolatedRotation(float alpha);
		glm::vec3					getInterpolatedScale(float alpha);
		AABB						getWorldAABB();
		const glm::vec3&			getTranslation() const;
		const glm::quat&			getRotation() const;
		const glm::vec3&			getScale() const;
		glm::mat4					getMatrix();
		
	};
}