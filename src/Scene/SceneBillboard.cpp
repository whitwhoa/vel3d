#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	std::unique_ptr<Mesh> Scene::loadBillboardMesh(const std::string& name, float width, float height)
	{
		if (!(width > 0.0f && height > 0.0f))
		{
			SPDLOG_DEBUG("Scene::loadBillboardMesh(): Billboard width and height must be positive.");
			return nullptr;
		}

		GeoPoolT<VtxPosNrmlTx>* gp = static_cast<GeoPoolT<VtxPosNrmlTx>*>(this->renderGeoPools[VTX_POS_NRML_TX].get());

		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(name);
		mesh->gp = gp;
		mesh->firstIndex = gp->indices.size();
		mesh->baseVertex = gp->vertexCount();
		mesh->flags = MESHFLAG_RENDERABLE | MESHFLAG_POOLED;


		const float halfWidth = width * 0.5f;
		const float halfHeight = height * 0.5f;

		// Front face is toward -Z (matches billboard code using forward = -dir)
		const glm::vec3 n(0.0f, 0.0f, -1.0f);

		VtxPosNrmlTx v0;
		v0.position = glm::vec3(-halfWidth, halfHeight, 0.0f);
		v0.normal = n;
		v0.textureCoords = glm::vec2(0.0f, 0.0f);
		gp->vertices.push_back(v0);

		VtxPosNrmlTx v1;
		v1.position = glm::vec3(-halfWidth, -halfHeight, 0.0f);
		v1.normal = n;
		v1.textureCoords = glm::vec2(0.0f, 1.0f);
		gp->vertices.push_back(v1);

		VtxPosNrmlTx v2;
		v2.position = glm::vec3(halfWidth, -halfHeight, 0.0f);
		v2.normal = n;
		v2.textureCoords = glm::vec2(1.0f, 1.0f);
		gp->vertices.push_back(v2);

		VtxPosNrmlTx v3;
		v3.position = glm::vec3(halfWidth, halfHeight, 0.0f);
		v3.normal = n;
		v3.textureCoords = glm::vec2(1.0f, 0.0f);
		gp->vertices.push_back(v3);

		// Flip indices so -Z is front (CCW when viewed from -Z)
		gp->indices.push_back(0);
		gp->indices.push_back(2);
		gp->indices.push_back(1);
		gp->indices.push_back(0);
		gp->indices.push_back(3);
		gp->indices.push_back(2);


		mesh->indexCount = mesh->gp->indices.size() - mesh->firstIndex;

		mesh->refreshAABB();

		return mesh;
	}

	Billboard* Scene::addBillboard(vel::Material* material, vel::Camera* parentCamera, float width, float height)
	{
		this->billboards.emplace_back(std::make_unique<Billboard>());

		Mesh* m = this->addMesh(std::move(this->loadBillboardMesh(name + "_mesh", width, height)));

		Actor* a = stage->addActor(name, m, material);
		a->setDynamic(true);

		return stage->addBillboard(std::make_unique<Billboard>(a, parentCamera));
	}

	// version of addBillboard that accepts a pointer to an existing mesh instead of creating one (so that for example
	// if we have 100 enemies that all use the same size billboard, they can all use the same mesh, and we don't have to 
	// make 100 extra calls into the graphics driver to swap vaos)
	Billboard* Scene::addBillboard(Material* material, Camera* parentCamera, Mesh* mesh)
	{
		Actor* a = stage->addActor(name, mesh, material);
		a->setDynamic(true);

		return stage->addBillboard(std::make_unique<Billboard>(a, parentCamera));
	}

	void Scene::updateBillboards()
	{
		for (auto& s : this->stages)
			s->updateBillboards();
	}


} // END NAMESPACE