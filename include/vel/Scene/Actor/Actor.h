#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <vel/Scene/Animation/SkelAnimator.h>
#include <vel/Scene/Mesh/Mesh.h>
#include <vel/Scene/Actor/Transform.h>
#include <vel/Scene/Actor/DrawBucketLocation.h>
#include <vel/Scene/Material.h>
#include <vel/Scene/Stage/Stage.h>

#include <vel/Util/slot_map.h>
#include <vel/Scene/Actor/FrameAnimator.h>



namespace vel
{
	typedef slot_handle actor_handle;

	enum ActFlg : uint32_t
	{
		ACTFLG_NONE = 0,
		ACTFLG_VISIBLE = 1 << 0,
		ACTFLG_DYNAMIC = 1 << 1,
		ACTFLG_LERPABLE = 1 << 2,
		ACTFLG_ANIMATED_MATERIAL = 1 << 4,
		ACTFLG_AMBIENT_CUBE = 1 << 5,
		ACTFLG_BILLBOARD = 1 << 6, // used in shader, do not change
		ACTFLG_BILLBOARD_LOCK_Y = 1 << 7 // used in shader, do not change
	};

	class Actor
	{
	private:
		Transform						transform;
		Transform						previousTransform;
	public:
		std::vector<std::pair<uint32_t, uint32_t>> activeBones; // the bones from the armature used by the mesh, .first = animator.renderModelMatrices index, .second = mesh.bones index
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
		
		std::vector<uint32_t>			materialIndices; // mesh.sections[n].materialIndex selects one slot in this vector.
		std::vector<DrawBucketLocation> drawBuckets; // one draw bucket per mesh section
		std::vector<FrameAnimator>		materialAnimators; // lockstep with materialIndices. Selects one texture inside the corresponding Material, or ignored (and zero used in its place if material not animated)

		glm::vec4						colorMultiplier;
		std::vector<glm::vec3>			ambientCube;
		texture_handle					lightmapTexture;

	private:
		void						_updatePrevTransform();
		
	public:
		Actor();
		//Actor(const Actor& original);
		//Actor& operator=(const Actor& a);

		void						setTranslation(glm::vec3 t);
		void						setRotation(float angle, glm::vec3 axis);
		void						setRotation(glm::quat r);
		void						appendRotation(float angle, glm::vec3 axis);
		void						setScale(glm::vec3 s);

		const Transform&			getTransform() const;
		const Transform&			getPreviousTransform() const;

		glm::vec3					getInterpolatedTranslation(float alpha);
		glm::quat					getInterpolatedRotation(float alpha);
		glm::vec3					getInterpolatedScale(float alpha);

		const glm::vec3&			getTranslation() const;
		const glm::quat&			getRotation() const;
		const glm::vec3&			getScale() const;

		glm::mat4					getMatrix();

		AABB						getWorldAABB();
	};
}