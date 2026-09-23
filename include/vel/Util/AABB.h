#pragma once

#include <vector>
#include <array>

#include <glm/glm.hpp>

namespace vel
{
	struct Mesh;

	class AABB
	{
	private:
		void							setCorners();
		void							initFromMeshVerts(const Mesh* mesh, const auto& verts); // abbreviated template

	public:
		glm::vec3						minEdge;
		glm::vec3						maxEdge;
		std::vector<glm::vec3>			corners;

		AABB();
		AABB(glm::vec3 min, glm::vec3 max);
		AABB(const std::vector<glm::vec3>& inputVectors);
		AABB(const Mesh* mesh);

		glm::vec3						getFarthestCorner();
		glm::vec3						getSize();
		glm::vec3						getHalfExtents();
		bool							contains(glm::vec3 v);

	};
}
