
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <glm/glm.hpp>

#include <spdlog/spdlog.h>

#include <vel/App.h>
#include <vel/Util/functions.h>
#include <vel/Util/Assert.h>
#include <vel/Scene/CollisionWorld/CollisionWorld.h>
#include <vel/Util/RaycastCallback.h>
#include <vel/Util/ConvexCastCallback.h>
#include <vel/Util/SimpleCollisionCallback.h>



namespace vel
{
	CollisionWorld::CollisionWorld(const std::string& name, float gravity) :
		nextCollisionShapeId(0),
		name(name),
		isActive(true),
		collisionConfiguration(new btDefaultCollisionConfiguration()),
		dispatcher(new btCollisionDispatcher(collisionConfiguration)),
		overlappingPairCache(new btDbvtBroadphase()),
		solver(new btSequentialImpulseConstraintSolver),
		dynamicsWorld(new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration)),
		camera(nullptr),
		collisionDebugDrawer(nullptr)
	{
		///////// added below to handle jitter when objects sliding across faces
		// https://stackoverflow.com/questions/25605659/avoid-ground-collision-with-bullet/25725502#25725502
		gContactAddedCallback = &CollisionWorld::contactAddedCallback;

		this->dynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
		
		btVector3 gravityVec(0.0f, gravity, 0.0f);
		this->dynamicsWorld->setGravity(gravityVec);
	}

	CollisionWorld::~CollisionWorld()
	{
		if (this->collisionDebugDrawer)
			delete this->collisionDebugDrawer;

		// Remove collision objects from the dynamics world and delete them
		for (int i = this->dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);

			if (body && body->getMotionState())
				delete body->getMotionState();

			this->dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		// Delete collision shapes
		for (auto& cs : this->collisionShapes)
		{
			delete cs.second;
			cs.second = nullptr;
		}

		this->collisionTriangleInfoMaps.clear();

		// Delete triangle meshes AFTER their associated collision shapes
		for (auto& tm : this->collisionTriangleMeshes)
		{
			delete tm.second;
			tm.second = nullptr;
		}

		delete this->dynamicsWorld;
		delete this->solver;
		delete this->overlappingPairCache;
		delete this->dispatcher;
		delete this->collisionConfiguration;
	}

	const std::string& CollisionWorld::getName()
	{
		return this->name;
	}

	void CollisionWorld::setCamera(Camera* c)
	{
		this->camera = c;
	}

	Camera* CollisionWorld::getCamera()
	{
		return this->camera;
	}

	bool CollisionWorld::getIsActive()
	{
		return this->isActive;
	}

	void CollisionWorld::setIsActive(bool b)
	{
		this->isActive = b;
	}

	void CollisionWorld::addCollisionObjectTemplate(const std::string& name, CollisionObjectTemplate cot)
	{
		this->collisionObjectTemplates[name] = cot;
	}
	
	CollisionObjectTemplate& CollisionWorld::getCollisionObjectTemplate(const std::string& name)
	{
		auto it = this->collisionObjectTemplates.find(name);
		if (it != this->collisionObjectTemplates.end())
			return it->second;

		VEL_ASSERT(false, ("CollisionWorld::getCollisionObjectTemplate() - no template with name of: " + name).c_str());
	}

	void CollisionWorld::useDebugDrawer(Shader* s, int debugMode)
	{
		VEL_ASSERT(!this->collisionDebugDrawer, "Debug drawer already initialized. Only one initialization allowed, or we leak memory, and this is not worth adding safety logic since it is intended for development debug");
		
		this->collisionDebugDrawer = new CollisionDebugDrawer();
		this->collisionDebugDrawer->setDebugMode(debugMode);
		this->collisionDebugDrawer->setShaderProgram(s);

		this->dynamicsWorld->setDebugDrawer(this->collisionDebugDrawer);
	}

	CollisionDebugDrawer* CollisionWorld::getDebugDrawer() //TODO should this really return nullptr OOORrrrrrrr??????
	{
		return this->collisionDebugDrawer;
	}

	bool CollisionWorld::getDebugEnabled()
	{
		return this->collisionDebugDrawer != nullptr;
	}

	void CollisionWorld::removeGhostObject(btPairCachingGhostObject* go)
	{
		this->dynamicsWorld->removeCollisionObject(go);
		delete go;
	}

	void CollisionWorld::removeRigidBody(btRigidBody* rb)
	{
		if (rb->getMotionState())
			delete rb->getMotionState();

		this->dynamicsWorld->removeCollisionObject(rb);

		delete rb;
	}

	bool CollisionWorld::contactAddedCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, partId1, index1);
		return true;
	}

	void CollisionWorld::addCollisionShape(const std::string& name, btCollisionShape* shape)
	{
		VEL_ASSERT(shape, "CollisionWorld::addCollisionShape(): Collision shape cannot be null.");

		auto [it, inserted] = this->collisionShapes.try_emplace(name, shape);

		VEL_ASSERT(inserted, ("CollisionWorld::addCollisionShape(): A collision shape named '" + name + "' already exists.").c_str());
	}

	btDiscreteDynamicsWorld* const	CollisionWorld::getDynamicsWorld()
	{
        return this->dynamicsWorld;
	}
    
	btCollisionShape* CollisionWorld::getCollisionShape(const std::string& name)
	{
		auto it = this->collisionShapes.find(name);
		if (it != this->collisionShapes.end())
			return it->second;

		VEL_ASSERT(false, ("CollisionWorld::getCollisionShape() - no collision shape with name of: " + name).c_str());
	}

	void CollisionWorld::addMeshTriangles(btTriangleMesh* triangleMesh, const Mesh* mesh, const auto& verts, const glm::mat4& transform, bool applyTransform)
	{
		const auto& indices = mesh->gp->indices;

		// Determine how many vertices this mesh references.
		uint32_t maxIndex = 0;

		for (size_t i = 0; i < mesh->indexCount; ++i)
			maxIndex = std::max(maxIndex, indices[mesh->firstIndex + i]);

		triangleMesh->preallocateVertices(maxIndex + 1);
		triangleMesh->preallocateIndices(mesh->indexCount);

		// Add each vertex once.
		for (size_t i = 0; i <= maxIndex; ++i)
		{
			glm::vec3 pos = verts[mesh->baseVertex + i].position;

			if (applyTransform)
				pos = glm::vec3(transform * glm::vec4(pos, 1.0f));

			triangleMesh->findOrAddVertex(glmToBulletVec3(pos), false);
		}

		// Add triangles using existing vertex indices.
		for (size_t i = 0; i < mesh->indexCount; i += 3)
		{
			size_t index = mesh->firstIndex + i;

			triangleMesh->addTriangleIndices(
				indices[index],
				indices[index + 1],
				indices[index + 2]
			);
		}
	}

	btCollisionShape* CollisionWorld::collisionShapeFromActor(Actor* actor, bool applyTransform)
	{
		if (actor->mesh == nullptr || actor->mesh->gp == nullptr || actor->mesh->indexCount == 0)
			return nullptr;

		auto* mesh = actor->mesh;
		std::string shapeName = mesh->name + "_shape";

		// Reuse existing collision geometry when no transform is baked into it.
		if (!applyTransform)
		{
			auto it = this->collisionShapes.find(shapeName);

			if (it != this->collisionShapes.end())
				return it->second;
		}
		else
		{
			// Transformed shapes are unique to their actor's current transform.
			shapeName += "_" + std::to_string(this->nextCollisionShapeId++);
		}

		auto transformMatrix = actor->getTransform().getMatrix(); // this ignores parenting

		btTriangleMesh* triangleMesh = new btTriangleMesh();

		switch (mesh->gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			this->addMeshTriangles(triangleMesh, mesh, static_cast<GeoPoolT<VtxPos>*>(mesh->gp)->vertices, transformMatrix, applyTransform);
			break;
		case VtxLayout::VTX_POS_NRML:
			this->addMeshTriangles(triangleMesh, mesh, static_cast<GeoPoolT<VtxPosNrml>*>(mesh->gp)->vertices, transformMatrix, applyTransform);
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			this->addMeshTriangles(triangleMesh, mesh, static_cast<GeoPoolT<VtxPosNrmlTx>*>(mesh->gp)->vertices, transformMatrix, applyTransform);
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			this->addMeshTriangles(triangleMesh, mesh, static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(mesh->gp)->vertices, transformMatrix, applyTransform);
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			this->addMeshTriangles(triangleMesh, mesh, static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(mesh->gp)->vertices, transformMatrix, applyTransform);
			break;
		default:
			delete triangleMesh;
			return nullptr;
		}

		btBvhTriangleMeshShape* bvhShape = new btBvhTriangleMeshShape(triangleMesh, true);
		bvhShape->setMargin(0);

		auto triangleInfoMap = std::make_unique<btTriangleInfoMap>();
		btGenerateInternalEdgeInfo(bvhShape, triangleInfoMap.get());

		auto [shapeIt, shapeInserted] = this->collisionShapes.try_emplace(shapeName, bvhShape);
		VEL_ASSERT(shapeInserted, ("CollisionWorld::collisionShapeFromActor(): A collision shape named '" + shapeName + "' already exists.").c_str());

		auto [meshIt, meshInserted] = this->collisionTriangleMeshes.try_emplace(shapeName, triangleMesh);
		VEL_ASSERT(meshInserted, ("CollisionWorld::collisionShapeFromActor(): A collision triangle mesh named '" + shapeName + "' already exists.").c_str());

		auto [infoIt, infoInserted] = this->collisionTriangleInfoMaps.try_emplace(bvhShape, std::move(triangleInfoMap));
		VEL_ASSERT(infoInserted, "CollisionWorld::collisionShapeFromActor(): Triangle info map already exists for collision shape.");

		return bvhShape;
	}

	btRigidBody* CollisionWorld::addStaticCollisionBody(Actor* actor, int collisionFilterGroup, int collisionFilterMask)
	{
		VEL_ASSERT(actor, "CollisionWorld::addStaticCollisionBody(): Actor cannot be null.");

		btCollisionShape* staticCollisionShape = this->collisionShapeFromActor(actor);
		
		VEL_ASSERT(staticCollisionShape, "CollisionWorld::addStaticCollisionBody(): Actor did not produce a valid collision shape.");

		btScalar mass(0);
		btVector3 localInertia(0, 0, 0);
		btDefaultMotionState* defaultMotionState = new btDefaultMotionState();
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, defaultMotionState, staticCollisionShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

		this->dynamicsWorld->addRigidBody(body, collisionFilterGroup, collisionFilterMask);

		return body;
	}

	std::optional<RaycastResult> CollisionWorld::rayTest(btVector3 from, btVector3 to, int collisionFilterMask, std::vector<btCollisionObject*> blackList)
	{
		RaycastCallback raycast = RaycastCallback(from, to, blackList);
		raycast.m_collisionFilterGroup = 1;
		raycast.m_collisionFilterMask = collisionFilterMask;
		this->dynamicsWorld->rayTest(from, to, raycast);

		if (!raycast.hasHit() || !raycast.m_collisionObject)
			return {};

		RaycastResult r;
		r.collisionObject = raycast.m_collisionObject;
		r.hitpoint = raycast.m_hitPointWorld;
		r.normal = raycast.m_hitNormalWorld.normalized();
		r.distance = btVector3(from - r.hitpoint).length();
		r.normalUpDot = r.normal.dot(btVector3(0, 1, 0));
		r.triangleIndex = raycast.m_triangleIndex;

		return r;
	}

	std::optional<ConvexCastResult> CollisionWorld::convexSweepTest(btConvexShape* castShape, btVector3 from, btVector3 to, int collisionFilterMask, std::vector<btCollisionObject*> blackList)
	{
		btTransform convexFromWorld;
		convexFromWorld.setIdentity();
		convexFromWorld.setOrigin(from);

		btTransform convexToWorld;
		convexToWorld.setIdentity();
		convexToWorld.setOrigin(to);

		ConvexCastCallback convexCast(from, to, blackList);
		convexCast.m_collisionFilterGroup = 1;
		convexCast.m_collisionFilterMask = collisionFilterMask;

		this->dynamicsWorld->convexSweepTest(castShape, convexFromWorld, convexToWorld, convexCast);

		if (!convexCast.hasHit() || !convexCast.m_hitCollisionObject)
			return {};

		ConvexCastResult ccr;
		ccr.collisionObject = convexCast.m_hitCollisionObject;
		ccr.hitpoint = convexCast.m_hitPointWorld;
		ccr.normal = convexCast.m_hitNormalWorld.normalized();
		ccr.normalUpDot = ccr.normal.dot(btVector3(0, 1, 0));
		
		return ccr;
	}

	bool CollisionWorld::getTriangleVertices(const btStridingMeshInterface* meshInterface, int triangleIndex, btVector3& v0, btVector3& v1, btVector3& v2, int& index0, int& index1, int& index2)
	{
		VEL_ASSERT(meshInterface, "CollisionWorld::getTriangleVertices(): Mesh interface cannot be null.");

		if (triangleIndex < 0)
			return false;

		int triangleOffset = 0;

		for (int part = 0; part < meshInterface->getNumSubParts(); ++part)
		{
			const unsigned char* vertexBase;
			int numVerts;
			PHY_ScalarType vertexType;
			int vertexStride;
			const unsigned char* indexBase;
			int indexStride;
			int numFaces;
			PHY_ScalarType indexType;

			meshInterface->getLockedReadOnlyVertexIndexBase(
				&vertexBase, numVerts, vertexType, vertexStride,
				&indexBase, indexStride, numFaces, indexType, part
			);

			if (triangleIndex >= triangleOffset + numFaces)
			{
				triangleOffset += numFaces;
				meshInterface->unLockReadOnlyVertexBase(part);
				continue;
			}

			const int localTriangleIndex = triangleIndex - triangleOffset;
			const unsigned char* triangleData = indexBase + localTriangleIndex * indexStride;

			VEL_ASSERT(
				indexType == PHY_INTEGER || indexType == PHY_SHORT || indexType == PHY_UCHAR,
				"CollisionWorld::getTriangleVertices(): Unsupported index format."
			);

			if (indexType == PHY_INTEGER)
			{
				const int* indices = reinterpret_cast<const int*>(triangleData);
				index0 = indices[0];
				index1 = indices[1];
				index2 = indices[2];
			}
			else if (indexType == PHY_SHORT)
			{
				const unsigned short* indices = reinterpret_cast<const unsigned short*>(triangleData);
				index0 = indices[0];
				index1 = indices[1];
				index2 = indices[2];
			}
			else
			{
				index0 = triangleData[0];
				index1 = triangleData[1];
				index2 = triangleData[2];
			}

			VEL_ASSERT(index0 >= 0 && index0 < numVerts && index1 >= 0 && index1 < numVerts && index2 >= 0 && index2 < numVerts, "CollisionWorld::getTriangleVertices(): Triangle contains an invalid vertex index.");
			VEL_ASSERT(vertexType == PHY_FLOAT || vertexType == PHY_DOUBLE, "CollisionWorld::getTriangleVertices(): Unsupported vertex format.");

			auto readVertex = [&](int index)
				{
					const unsigned char* vertexData = vertexBase + index * vertexStride;

					if (vertexType == PHY_FLOAT)
					{
						const float* vertex = reinterpret_cast<const float*>(vertexData);
						return btVector3(vertex[0], vertex[1], vertex[2]);
					}

					const double* vertex = reinterpret_cast<const double*>(vertexData);
					return btVector3(static_cast<btScalar>(vertex[0]), static_cast<btScalar>(vertex[1]), static_cast<btScalar>(vertex[2]));
				};

			v0 = readVertex(index0);
			v1 = readVertex(index1);
			v2 = readVertex(index2);

			meshInterface->unLockReadOnlyVertexBase(part);
			return true;
		}

		SPDLOG_DEBUG("CollisionWorld::getTriangleVertices(): Invalid triangle index: {}", triangleIndex);
		return false;
	}

}