#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>

#include <glm/glm.hpp>


#include <vel/Util/AABB.h>
#include <vel/Render/Shader.h>

#include <vel/Scene/Stage/Actor/Mesh/MeshBone.h>
#include <vel/Scene/Stage/Actor/Mesh/GeoPool.h>


namespace vel
{
	struct Mesh
	{
		std::string name = "";

		uint32_t firstIndex = 0; // Where are my indices?
		uint32_t indexCount = 0; // How many indices do I draw?
		uint32_t baseVertex = 0; // Where are my vertices?
		GeoPool* gp = nullptr; // Mesh data, and optionally which VBO/EBO/VAO the above refers to

		std::vector<MeshBone> bones;

		AABB aabb;


					Mesh(std::string name);

		bool		isRenderable() const;
		MeshBone*	getBone(std::string boneName);
		void		refreshAABB();

	};
    
}