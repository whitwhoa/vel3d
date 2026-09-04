#include <glm/gtx/matrix_decompose.hpp>

#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Camera/Camera.h>



using json = nlohmann::json;

namespace vel
{
	unsigned int Camera::nextCameraId = 1;

	Camera::Camera(CameraType type) :
		id(Camera::nextCameraId++),
		resolution(Runtime::_window->getResolution()),
		previousResolution(glm::ivec2(0, 0)),
		renderTarget(Runtime::_gpu->createRenderTarget((this->id + "_RT"), resolution.x, resolution.y)),
		type(type),
		fovScale(75.0f),
		nearPlane(0.1f),
		farPlane(100.0f),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		lookAt(glm::vec3(0.0f, 0.0f, 0.0f)),
		up(glm::vec3(0.0f, 1.0f, 0.0f)),
		gpuData({}),
		finalRenderCam(true),
		resolutionFixed(false)
	{}

	unsigned int Camera::getId() const
	{
		return this->id;
	}

	void Camera::update()
	{
		if (this->resolution.x == 0 || this->resolution.y == 0)
			return; // do not update FBOs when window is minimized

		glm::ivec2 currentResolution = this->resolution;

		// if current viewport size does not equal the previous tick's viewport size, we have to rebuild the render target
		if (currentResolution != this->previousResolution)
		{
			SPDLOG_DEBUG("Camera::update(): viewport size altered");

			RenderTarget rt = Runtime::_gpu->createRenderTarget((this->getId() + "_RT"), currentResolution.x, currentResolution.y);
			Runtime::_gpu->clearRenderTarget(&this->renderTarget);
			this->renderTarget = rt;
		}

		this->previousResolution = currentResolution;

		//
		// Update view matrix
		//
		if (type == CameraType::SCREEN_SPACE)
			this->gpuData.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-this->position.x, -this->position.y, 0.0f)); // allow movement
		else
			this->gpuData.viewMatrix = glm::lookAt(this->position, this->lookAt, this->up);

		//
		// Update projection matrix
		//
		glm::vec2 vps = this->resolution;

		if (this->type == CameraType::ORTHOGRAPHIC)
		{
			float aspect = vps.x / vps.y;
			float sizeX = fovScale * aspect;
			float sizeY = fovScale;
			this->gpuData.projectionMatrix = glm::ortho(-sizeX, sizeX, -sizeY, sizeY, this->nearPlane, this->farPlane);
		}
		else if (this->type == CameraType::SCREEN_SPACE)
		{
			// Set up a screen space orthographic projection that maps pixel coordinates.
			//this->projectionMatrix = glm::ortho(0.0f, vps.x, vps.y, 0.0f, -1.f, 1.f); // top left
			//this->projectionMatrix = glm::ortho(0.0f, vps.x, 0.0f, vps.y, -1.f, 1.f); // bottom left
			this->gpuData.projectionMatrix = glm::ortho(0.f, vps.x, vps.y, 0.f, -1.f, 1.f); // top left
		}
		else
		{
			if (vps.x > 0 && vps.y > 0) // crashes when windowing out of fullscreen if this condition is not checked
			{
				this->gpuData.projectionMatrix = glm::perspective(glm::radians(this->fovScale),
					vps.x / vps.y, this->nearPlane, this->farPlane);
			}
		}
	}

}