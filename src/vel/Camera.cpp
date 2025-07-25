
#define GLM_FORCE_ALIGNED_GENTYPES
#include "glm/gtx/matrix_decompose.hpp"



#include "vel/App.h"
#include "vel/Camera.h"
#include "vel/Log.h"


using json = nlohmann::json;

namespace vel
{
	Camera::Camera(const std::string& name, CameraType type) :
		name(name),
		type(type),
		gpu(nullptr),
		resolution(glm::ivec2(1280, 720)),
		resolutionFixed(false),
		previousResolution(glm::ivec2(0, 0)),
		nearPlane(0.1f),
		farPlane(100.0f),
		fovScale(75.0f),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		lookAt(glm::vec3(0.0f, 0.0f, 0.0f)),
		up(glm::vec3(0.0f, 1.0f, 0.0f)),
		viewMatrix(glm::mat4(1.0f)),
		projectionMatrix(glm::mat4(1.0f)),
		finalRenderCam(true)
	{

	}

	glm::vec3 Camera::getLookAt()
	{
		return this->lookAt;
	}

	RenderTarget* Camera::getRenderTarget()
	{
		return &this->renderTarget.value();
	}

	void Camera::setRenderTarget(RenderTarget rt)
	{
		this->renderTarget = rt;
	}

	bool Camera::isFinalRenderCam()
	{
		return this->finalRenderCam;
	}

	void Camera::setFinalRenderCam(bool b)
	{
		this->finalRenderCam = b;
	}

	void Camera::setResolution(int width, int height)
	{
		this->resolution = glm::ivec2(width, height);
	}

	void Camera::setFovOrScale(float fos)
	{
		this->fovScale = fos;
	}

	void Camera::setNearPlane(float np)
	{
		this->nearPlane = np;
	}

	void Camera::setFarPlane(float fp)
	{
		this->farPlane = fp;
	}

	void Camera::setType(CameraType type)
	{
		this->type = type;
	}

	const std::string& Camera::getName() const
	{
		return this->name;
	}

	glm::ivec2 Camera::getResolution()
	{
		return this->resolution;
	}

	float Camera::getFovScale()
	{
		return this->fovScale;
	}

	void Camera::updateProjectionMatrix()
	{
		glm::vec2 vps = this->resolution;

		if (this->type == CameraType::ORTHOGRAPHIC)
		{
			float aspect = vps.x / vps.y;
			float sizeX = fovScale * aspect;
			float sizeY = fovScale;
			this->projectionMatrix = glm::ortho(-sizeX, sizeX, -sizeY, sizeY, this->nearPlane, this->farPlane);
		}
		else if (this->type == CameraType::SCREEN_SPACE)
		{
			// Set up a screen space orthographic projection that maps pixel coordinates.
			// Here, the top-left corner is (0,0) and bottom-right is (vps.x, vps.y)
			this->projectionMatrix = glm::ortho(0.0f, vps.x, vps.y, 0.0f, this->nearPlane, this->farPlane); // top left
			//this->projectionMatrix = glm::ortho(0.0f, vps.x, 0.0f, vps.y, this->nearPlane, this->farPlane); // bottom left
		}
		else
		{
			if (vps.x > 0 && vps.y > 0) // crashes when windowing out of fullscreen if this condition is not checked
			{
				this->projectionMatrix = glm::perspective(glm::radians(this->fovScale),
					vps.x / vps.y, this->nearPlane, this->farPlane);
			}
		}

	}

	void Camera::updateViewMatrix()
	{
		if (type == CameraType::SCREEN_SPACE)
		{
			this->viewMatrix = glm::mat4(1.0f);
			return;
		}

		this->viewMatrix = glm::lookAt(this->position, this->lookAt, this->up);
	}

	void Camera::setFixedResolution(bool b)
	{
		this->resolutionFixed = b;
	}

	bool Camera::getFixedResolution()
	{
		return this->resolutionFixed;
	}

	void Camera::update()
	{
		if (this->resolution.x == 0 || this->resolution.y == 0)
			return; // do not update FBOs when window is minimized

		glm::ivec2 currentResolution = this->resolution;

		//std::cout << this->resolution.x << "," << this->resolution.y << "\n";

		// if current viewport size does not equal the previous tick's viewport size, we have to rebuild the render target
		if (currentResolution != this->previousResolution)
		{
			LOG_TO_CLI_AND_FILE("viewport size altered\n");

			RenderTarget rt = this->gpu->createRenderTarget((this->getName() + "_RT"), currentResolution.x, currentResolution.y);
			this->gpu->clearRenderTarget(&this->renderTarget.value());
			this->renderTarget = rt;
		}

		this->previousResolution = currentResolution;

		this->updateViewMatrix();
		this->updateProjectionMatrix();
	}

	glm::mat4 Camera::getViewMatrix()
	{
		return this->viewMatrix;
	}

	glm::mat4 Camera::getProjectionMatrix()
	{
		return this->projectionMatrix;
	}

	void Camera::setPosition(float x, float y, float z)
	{
		this->position = glm::vec3(x, y, z);
	}

	void Camera::setPosition(glm::vec3 position)
	{
		this->position = position;
	}

	void Camera::setLookAt(float x, float y, float z)
	{
		this->lookAt = glm::vec3(x, y, z);
	}

	void Camera::setLookAt(glm::vec3 direction)
	{
		this->lookAt = direction;
	}

	glm::vec3 Camera::getPosition()
	{
		return this->position;
	}

	void Camera::setGpu(GPU* gpu)
	{
		this->gpu = gpu;
	}

}