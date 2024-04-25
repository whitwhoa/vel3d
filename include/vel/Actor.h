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
#include "vel/Material.h"


namespace vel
{
	class	Stage;
	class	CollisionWorld;

	class Actor
	{
	private:
		std::string										name;
		bool											visible;
		bool											dynamic;

		Transform										transform;
		std::optional<Transform>						previousTransform;

		Actor*											parentActor;
		ArmatureBone*									parentArmatureBone;
		std::vector<Actor*>								childActors;

		Shader*											shader;

		Armature*										armature;
		std::vector<std::pair<size_t, unsigned int>>	activeBones; // the bones from the armature that are actually used by the mesh, 
																	// the glue between an armature and a mesh (index is mesh bone index, value is armature bone index)
		
		Mesh*											mesh;
		std::optional<Material>							material; // must be a copy
		Texture*										lightMapTexture;
		glm::vec4										color; // blends with material
		std::vector<glm::vec3>							giColors;// -x, +x, -y, +y, -z, +z
		
		void*											userPointer;

	public:
		Actor(std::string name);
		Actor											cleanCopy(std::string newName);
		void											setDynamic(bool dynamic);

		void											setName(std::string newName);
		const std::string								getName() const;

		void											setShader(Shader* t);
		Shader*											getShader();

		void											setMesh(Mesh* m);
		Mesh*											getMesh();

		void											setArmature(Armature* arm);
		Armature*										getArmature();

		void											setColor(glm::vec4 c);
		const glm::vec4&								getColor();

		void											setMaterial(Material m);
		std::optional<Material>&						getMaterial();

		void											setLightMapTexture(Texture* t);
		Texture*										getLightMapTexture();
		


		void											setVisible(bool v);
		const bool										isVisible() const;
		const bool										isAnimated() const;
		const bool										isDynamic() const;
		

		const std::vector<std::pair<size_t, unsigned int>>& getActiveBones() const;
		void											setActiveBones(std::vector<std::pair<size_t, unsigned int>> activeBones);
		void											setParentActor(Actor* a);
		void											setParentArmatureBone(ArmatureBone* b);
		void											addChildActor(Actor* a);
		Transform&										getTransform();
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

		void											updateGIColors(std::vector<glm::vec3>& colors);

		void											setEmptyGIColors();
		std::vector<glm::vec3>&							getGIColors();
	};
}