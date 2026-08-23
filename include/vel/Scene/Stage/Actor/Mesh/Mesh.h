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
	enum MeshFlag : uint32_t
	{
		MESHFLAG_NONE = 0,
		MESHFLAG_RENDERABLE = 1 << 0,
	};

	struct Mesh
	{
		AABB aabb;
		std::string name = "";
		std::vector<MeshBone> bones;
		GeoPool* gp = nullptr; // Mesh data, and optionally which VBO/EBO/VAO ints refers to
		uint32_t firstIndex = 0; // Where are my indices?
		uint32_t indexCount = 0; // How many indices do I draw?
		uint32_t baseVertex = 0; // Where are my vertices?
		uint32_t flags = 0;
		

					Mesh(const std::string& name);

		bool		isRenderable() const;
		MeshBone*	getBone(const std::string& boneName);
		void		refreshAABB(); // must be called manually when mesh geometry is updated

	};
    
}