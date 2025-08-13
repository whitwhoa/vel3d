#include "vel/Billboard.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vel 
{

	Billboard::Billboard(vel::Actor* billboardActor, vel::Camera* parentCamera, bool lockY) : 
		billboardActor(billboardActor), 
		parentCamera(parentCamera), 
		lockY(lockY) 
	{}

	vel::Actor* Billboard::getActor() const
	{
		return this->billboardActor;
	}

	void Billboard::update()
	{
		if (!billboardActor || !parentCamera) 
			return;

		// Positions
		const glm::vec3 camPos = parentCamera->getPosition();
		const glm::vec3 objPos = billboardActor->getTransform().getTranslation();

		// Direction from object to camera
		glm::vec3 dir = camPos - objPos;

		// Lock rotation around X/Z if requested (i.e., yaw-only billboarding)
		if (lockY) 
			dir.y = 0.0f;

		// Handle degenerate cases
		const float eps = 1e-6f;
		if (glm::length2(dir) < eps) 
			return; // camera is on top of the object (or nearly so); leave rotation unchanged
		
		dir = glm::normalize(dir);
		const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

		// Build an orthonormal basis (Right, Up, Forward)
		// Forward should be the direction the *object's -Z* points in world space.
		const glm::vec3 forward = -dir;
		glm::vec3 right = glm::cross(worldUp, forward);
		if (glm::length2(right) < eps) 
		{
			// forward is parallel to up; pick an arbitrary orthogonal right
			right = glm::cross(glm::vec3(1, 0, 0), forward);
			if (glm::length2(right) < eps) 
				right = glm::cross(glm::vec3(0, 0, 1), forward);
		}
		right = glm::normalize(right);
		const glm::vec3 up = glm::normalize(glm::cross(forward, right));

		// Compose a rotation matrix where columns are the basis vectors (column-major)
		glm::mat3 R(1.0f);
		R[0] = right;    // X
		R[1] = up;       // Y
		R[2] = forward;  // Z (remember this corresponds to local -Z facing camera)

		// Convert to quaternion and apply to the actor’s transform
		const glm::quat q = glm::quat_cast(R);
		billboardActor->getTransform().setRotation(q);
	}

}
