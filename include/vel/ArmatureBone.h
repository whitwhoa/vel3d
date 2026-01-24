#pragma once

#include <string>
#include <vector>
#include <optional>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"




namespace vel
{
	class Actor;
	class Armature;

	struct ArmatureBone
	{
		Armature*		parentArmature;
		size_t			parentArmatureIndex;
		std::string		name;
		std::string		parentName;
		size_t			parent;

		glm::vec3		translation;
		glm::quat		rotation;
		glm::vec3		scale;
		glm::mat4		matrix;

		// list of actors that are parented to this bone, useful
		// if for example we have an armature that has objects parented to it,
		// such as a character object with weapon actors parented at various
		// locations, and then that that character object gets deleted, but the
		// weapon objects still hold pointers to this bone, we need to know that
		// we need to clear those ArmatureBone pointers on the weapon objects so they
		// are no longer parented to the armature which no longer exists
		std::vector<Actor*> childActors;
	};
}