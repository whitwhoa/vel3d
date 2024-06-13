#pragma once

#include <optional>
#include <string>

#include "glm/glm.hpp"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "vel/Armature.h"
#include "vel/ArmatureBone.h"
#include "vel/Mesh.h"
#include "vel/Transform.h"
//#include "vel/Material.h"



namespace vel
{
	class	Stage;
	class	CollisionWorld;
	class	Material;


	class Actor
	{
	private:
		//static unsigned int copyCount;
		//static unsigned int getNextCopyCount();

		std::string										name;
		bool											visible;
		bool											dynamic;

		Transform										transform;
		std::optional<Transform>						previousTransform;

		Actor*											parentActor;
		ArmatureBone*									parentArmatureBone;
		std::vector<Actor*>								childActors;

		Armature*										armature;
		std::vector<std::pair<size_t, unsigned int>>	activeBones; // the bones from the armature that are actually used by the mesh, 
																	// the glue between an armature and a mesh (index is mesh bone index, value is armature bone index)
		
		Mesh*											mesh;

		// std::unique_ptr<Material> so we can have polymorphism, copy constructor and copy assignment operators overwritten to perform clone
		// as each actor needs it's own copy of it's material
		std::unique_ptr<Material>						material; // actor must own it's own copy of a material because of animators
		
		void*											userPointer;

	public:
		Actor(std::string name);

		
		Actor(const Actor& original); // Copy constructor
		Actor& operator=(const Actor& a); // Copy assignment operator



		void											setDynamic(bool dynamic);

		void											setName(std::string newName);
		const std::string								getName() const;

		void											setMesh(Mesh* m);
		Mesh*											getMesh();
		Mesh*											getMesh() const;

		void											setArmature(Armature* arm);
		Armature*										getArmature();
		Armature*										getArmature() const;

		void											setMaterial(Material* m);
		Material*										getMaterial();
		Material*										getMaterial() const;
		


		void											setVisible(bool v);
		const bool										isVisible() const;
		const bool										isAnimated() const;
		const bool										isDynamic() const;
		

		const std::vector<std::pair<size_t, unsigned int>>& getActiveBones() const;
		void											setActiveBones(std::vector<std::pair<size_t, unsigned int>> activeBones);

		void											setParentActor(Actor* a);
		void											setParentArmatureBone(ArmatureBone* b);

		void											addChildActor(Actor* a);
		std::vector<Actor*>&							getChildActors();

		Transform&										getTransform();
		const Transform&								getTransform() const;

		std::optional<Transform>&						getPreviousTransform();
		void											updatePreviousTransform();
		void											clearPreviousTransform();

		glm::mat4										getWorldMatrix();
		glm::mat4										getWorldRenderMatrix(float alpha); // contains logic for interpolation
		glm::vec3										getInterpolatedTranslation(float alpha);
		glm::quat										getInterpolatedRotation(float alpha);
		glm::vec3										getInterpolatedScale(float alpha);

		void											removeParentActor(bool calledFromRemoveChildActor = false);
		void											removeChildActor(Actor* a, bool calledFromRemoveParentActor = false);		

		void*											getUserPointer();
		void											setUserPointer(void* p);
	};
}