#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>

#include <vel/Scene/Camera/RenderTarget.h>

namespace vel
{
	enum class CameraType {
		ORTHOGRAPHIC,
		SCREEN_SPACE,
		PERSPECTIVE
	};

	struct CameraGpuData
	{
		glm::mat4 viewMatrix = glm::mat4(1.f);
		glm::mat4 projectionMatrix = glm::mat4(1.f);
	};

	class Camera
	{
	private:
		static unsigned int		nextCameraId;
		unsigned int			id;

	public:
		glm::ivec2				resolution;
		glm::ivec2				previousResolution;
		RenderTarget			renderTarget;
		CameraType              type;
		float                   fovScale;
		float                   nearPlane;
		float                   farPlane;
		glm::vec3			    position;
		glm::vec3			    lookAt;
		glm::vec3			    up;
		CameraGpuData			gpuData;
		bool					finalRenderCam;
		bool					resolutionFixed;

		Camera(CameraType type);
		unsigned int			getId() const;
		void                    update();
	};
}