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
		std::string				name;
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
		

		void                    updateViewMatrix();
		void                    updateProjectionMatrix();

	public:
		Camera(const std::string& name, CameraType type);
		void                    update();
		const std::string&		getName() const;
		glm::mat4               getViewMatrix();
		glm::mat4               getProjectionMatrix();
		glm::vec3               getPosition();
		glm::ivec2				getResolution();
		glm::vec3				getLookAt();
		void                    setPosition(float x, float y, float z);
		void                    setPosition(glm::vec3 position);
		void                    setLookAt(float x, float y, float z);
		void                    setLookAt(glm::vec3 direction);
		void					setType(CameraType type);
		void					setNearPlane(float np);
		void					setFarPlane(float fp);
		void					setFovOrScale(float fos);

		void					setResolution(int width, int height);
		void					setFixedResolution(bool b); // if resolution fixed, then it's not updated when window size is altered dynamically by user
		bool					getFixedResolution();

		void					setFinalRenderCam(bool b); // whether or not this camera is used to draw to screen buffer
		bool					isFinalRenderCam();

		RenderTarget&			getRenderTarget();

		float					getFovScale();

		CameraGpuData&			getGpuData();
	};
}