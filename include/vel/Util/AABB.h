#pragma once

#include <vector>
#include <array>

#include <glm/glm.hpp>

namespace vel
{
	class AABB
	{
	private:
		glm::vec3						minEdge;
		glm::vec3						maxEdge;
		std::vector<glm::vec3>			corners;

	public:
		
		// Construct using a vector of vertices, where min/max edges are generated
		// as the encompassing volumn of vector of vertices
		AABB(const std::vector<glm::vec3>& inputVectors);

		// Construct AABB using a min vector and max vector
		AABB(glm::vec3 min, glm::vec3 max);

		// Default
		AABB();
		
		const std::vector<glm::vec3>&	getCorners();
		glm::vec3						getFarthestCorner();
		glm::vec3						getSize();
		glm::vec3						getHalfExtents();
		glm::vec3						getMinEdge();
		glm::vec3						getMaxEdge();

		bool							contains(glm::vec3 v);

	};
}
