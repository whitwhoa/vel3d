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

#include <vel/Util/slot_map.h>



namespace vel
{
	class Actor
	{
	private:
		static unsigned int			nextActorId;

		uint32_t					lastTransformUpdateTick;
		unsigned int				id;
		Transform					transform;
		Transform					previousTransform;
		std::vector<std::pair<unsigned int, unsigned int>> activeBones; // the bones from the armature used by the mesh, .first = animator.renderModelMatrices index, .second = mesh.bones index

		void						_updatePrevTransform();


		// TODO: move to HeadlessScene
		void						_removeParentActor();
		void						_removeChildActor(Actor* child);
		// END
		
		

	public:
		void*						userPointer;
		Material*					material;
		Mesh*						mesh;
		SkelAnimator*				animator;
		std::optional<slot_handle>	parentActor; // If has value, this actor is a child of the actor referenced by slot_handle
		std::vector<slot_handle>	childActors; // If size() > 0, this actor is a parent to all actors referenced by contained slot_handles
		int							parentActorBone; // Used in conjunction with parentActor, when present (> -1)
		bool						visible;
		bool						dynamic;
		bool						lerpable;

									Actor();
									Actor(const Actor& original);
									Actor& operator=(const Actor& a);

		// TODO: move to Scene
		void						setDynamic(bool dynamic, bool lerpable = true);
		void						setVisible(bool v);
		// END

		// TODO: move to HeadlessScene
		bool						setAnimator(SkelAnimator* a);
		void						setParentActor(Actor* a);
		void						setParentActorBone(Actor* a, int boneId);
		void						addChildActor(Actor* a);
		void						removeParentActor();
		void						removeChildActor(Actor* child);
		// END


		void						setTranslation(glm::vec3 t);
		void						setRotation(float angle, glm::vec3 axis);
		void						setRotation(glm::quat r);
		void						appendRotation(float angle, glm::vec3 axis);
		void						setScale(glm::vec3 s);

		unsigned int				getId() const;
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