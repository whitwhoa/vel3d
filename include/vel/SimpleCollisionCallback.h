#pragma once

#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"

namespace vel
{
    struct SimpleCollisionCallback : public btCollisionWorld::ContactResultCallback
    {
        bool collided = false;

        bool needsCollision(btBroadphaseProxy* proxy0) const override {
            return true; // WARNING: ignores filter masks; use only to debug
        }

        btScalar addSingleResult(btManifoldPoint& cp,
            const btCollisionObjectWrapper* colObj0Wrap, int, int,
            const btCollisionObjectWrapper* colObj1Wrap, int, int) override
        {
            collided = true;
            return 0;
        }
    };
}