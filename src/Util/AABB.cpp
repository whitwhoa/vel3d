
#include <vel/Util/AABB.h>
#include <vel/Scene/Mesh/Mesh.h>

namespace vel
{
	AABB::AABB() :
		minEdge(glm::vec3(0.f)),
		maxEdge(glm::vec3(0.f))
	{
		setCorners();
	}

	AABB::AABB(glm::vec3 min, glm::vec3 max) :
		minEdge(min),
		maxEdge(max)
	{
		setCorners();
	}

	AABB::AABB(const std::vector<glm::vec3>& inputVectors)
	{
		bool firstPass = true;

		for (auto& v : inputVectors)
		{
			if (firstPass)
			{
				firstPass = false;
				minEdge = v;
				maxEdge = v;
				continue;
			}

			if (v.x > maxEdge.x)
				maxEdge.x = v.x;
			if (v.y > maxEdge.y)
				maxEdge.y = v.y;
			if (v.z > maxEdge.z)
				maxEdge.z = v.z;

			if (v.x < minEdge.x)
				minEdge.x = v.x;
			if (v.y < minEdge.y)
				minEdge.y = v.y;
			if (v.z < minEdge.z)
				minEdge.z = v.z;
		}

		setCorners();
	};

	AABB::AABB(const Mesh* mesh) :
		minEdge(glm::vec3(0.0f)),
		maxEdge(glm::vec3(0.0f))
	{
		switch (mesh->gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			initFromMeshVerts(mesh, static_cast<GeoPoolT<VtxPos>*>(mesh->gp)->vertices);
			break;
		case VtxLayout::VTX_POS_NRML:
			initFromMeshVerts(mesh, static_cast<GeoPoolT<VtxPosNrml>*>(mesh->gp)->vertices);
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			initFromMeshVerts(mesh, static_cast<GeoPoolT<VtxPosNrmlTx>*>(mesh->gp)->vertices);
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			initFromMeshVerts(mesh, static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(mesh->gp)->vertices);
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			initFromMeshVerts(mesh, static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(mesh->gp)->vertices);
			break;
		}

		setCorners();
	}

	void AABB::initFromMeshVerts(const Mesh* mesh, const auto& verts)
	{
		if (mesh->indexCount == 0)
			return;

		const auto& indices = mesh->gp->indices;

		size_t firstIndex = mesh->firstIndex;
		size_t baseVertex = mesh->baseVertex;

		const auto& firstPos = verts[baseVertex + indices[firstIndex]].position;

		minEdge = firstPos;
		maxEdge = firstPos;

		for (size_t i = 1; i < mesh->indexCount; ++i)
		{
			const auto& pos = verts[baseVertex + indices[firstIndex + i]].position;

			minEdge = glm::min(minEdge, pos);
			maxEdge = glm::max(maxEdge, pos);
		}
	}

	void AABB::setCorners()
	{
		// calculate all eight corner vectors
		corners.push_back(maxEdge);
		corners.push_back(minEdge);
		corners.push_back(glm::vec3(minEdge.x, maxEdge.y, maxEdge.z));
		corners.push_back(glm::vec3(minEdge.x, minEdge.y, maxEdge.z));
		corners.push_back(glm::vec3(maxEdge.x, minEdge.y, maxEdge.z));
		corners.push_back(glm::vec3(maxEdge.x, maxEdge.y, minEdge.z));
		corners.push_back(glm::vec3(minEdge.x, maxEdge.y, minEdge.z));
		corners.push_back(glm::vec3(maxEdge.x, minEdge.y, minEdge.z));
	}

	glm::vec3 AABB::getSize()
	{
		return glm::vec3(
			fabsf(maxEdge.x - minEdge.x),
			fabsf(maxEdge.y - minEdge.y),
			fabsf(maxEdge.z - minEdge.z)
		);
	}

	glm::vec3 AABB::getHalfExtents()
	{
		return getSize() * 0.5f;
	}

	glm::vec3 AABB::getFarthestCorner()
	{
		float checkVal = 0.0f;
		glm::vec3 returnVector = glm::vec3(0.f);

		for (auto& c : corners)
		{
			float cornerLength = glm::length(c);
			if (cornerLength > checkVal)
			{
				checkVal = cornerLength;
				returnVector = c;
			}
		}

		return returnVector;
	}

	bool AABB::contains(glm::vec3 v)
	{
		return v.x >= minEdge.x && v.x <= maxEdge.x &&
			v.y >= minEdge.y && v.y <= maxEdge.y &&
			v.z >= minEdge.z && v.z <= maxEdge.z;
	}

}