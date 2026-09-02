#pragma once

#include <string>

#include <glm/glm.hpp>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <vel/Scene/Animation/SkelAnimator.h>
#include <vel/Scene/Mesh/Mesh.h>
#include <vel/Scene/Actor/Transform.h>
#include <vel/Scene/Material.h>



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

		Actor*						parentActor; // If not null, this actor is a child of the actor pointed to by parentActor
		std::vector<Actor*>			childActors; // If size() > 0, this actor is a parent pointed to by all contained Actors
		std::vector<std::pair<unsigned int, unsigned int>> activeBones; // the bones from the armature used by the mesh, .first = animator.renderModelMatrices index, .second = mesh.bones index
		
		int							parentActorBone; // Used in conjunction with parentActor, when present (> -1)
		bool						visible;
		bool						dynamic;
		bool						lerpable;

		void						_removeParentActor();
		void						_removeChildActor(Actor* child);
		void						_updatePrevTransform();
		

	public:
		void*						userPointer;
		std::unique_ptr<Material>	material;
		Mesh*						mesh;
		SkelAnimator*				animator;

														Actor();
														Actor(const Actor& original);
														Actor& operator=(const Actor& a);

		void											setDynamic(bool dynamic, bool lerpable = true);
		bool											setAnimator(SkelAnimator* a);
		void											setVisible(bool v);
		void											setParentActor(Actor* a);
		void											setParentActorBone(Actor* a, int boneId);
		void											addChildActor(Actor* a);
		void											removeParentActor();
		void											removeChildActor(Actor* child);
		void											setTranslation(glm::vec3 t);
		void											setRotation(float angle, glm::vec3 axis);
		void											setRotation(glm::quat r);
		void											appendRotation(float angle, glm::vec3 axis);
		void											setScale(glm::vec3 s);


		unsigned int									getId() const;
		bool											isVisible() const;
		bool											isAnimated() const;
		bool											isDynamic() const;
		bool											isLerpable() const;
		std::vector<Actor*>&							getChildActors();
		const std::vector<std::pair<unsigned int, unsigned int>>& getActiveBones() const;
		const Transform&								getTransform() const;
		const Transform&								getPreviousTransform() const;
		glm::mat4										getWorldMatrix();
		glm::mat4										getWorldRenderMatrix(float alpha); // contains logic for interpolation
		glm::vec3										getInterpolatedTranslation(float alpha);
		glm::quat										getInterpolatedRotation(float alpha);
		glm::vec3										getInterpolatedScale(float alpha);
		AABB											getWorldAABB();
		const glm::vec3&								getTranslation() const;
		const glm::quat&								getRotation() const;
		const glm::vec3&								getScale() const;
		glm::mat4										getMatrix();
		
	};
}