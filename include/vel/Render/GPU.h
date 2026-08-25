#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <cstdint>

#include <glm/glm.hpp>

#include <vel/Scene/Stage/Actor/Mesh/Mesh.h>
#include <vel/Scene/Stage/Actor/Material/Texture/Texture.h>
#include <vel/Scene/Stage/Actor/Material/Material.h>
#include <vel/Physics/CollisionDebugDrawer.h>
#include <vel/Scene/Stage/Actor/Font/FontBitmap.h>

#include <vel/Render/Shader.h>
#include <vel/Render/Camera.h>
#include <vel/Render/RenderTarget.h>
#include <vel/Render/FinalRenderTarget.h>

struct __GLsync;
typedef __GLsync* GLsync;

typedef uint64_t GLuint64;


namespace vel
{
	class GPU
	{
	private:
		glm::vec4							zeroFillerVec;
		glm::vec4							oneFillerVec;
		glm::vec4							activeClearColorValues;

		std::unique_ptr<Mesh>				screenSpaceMesh;
		std::unique_ptr<Shader>				screenShader;
		std::unique_ptr<Shader>				postShader;
		std::unique_ptr<Shader>				compositeShader;
		RenderTarget*						activeRenderTarget;
		Shader*								activeShader;
		Mesh*								activeMesh;
		Material*							activeMaterial;
		glm::ivec2							activeCameraViewportSize;
		GLsync								prevFrameFence;

		unsigned int						bonesUBO;
		unsigned int						texturesUBO;
		unsigned int						lightmapTextureUBO;
		int									activeFramebuffer;
		bool								useFXAA;

		void								initBoneUBO();
		void								initTextureUBO();
		void								initLightMapTextureUBO();
		void								initScreenSpaceMesh();
		void								initShaders();

		void								bindFrameBuffer(unsigned int fbo);

	public:
		GPU(bool fxaa = false);
		~GPU();
		GPU(GPU&&) = default;
        void								enableDepthTest();
		void								clearBuffers(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);
		void								drawLinesOnly();

		
		const Shader* const					getActiveShader() const;
		const Mesh*	const					getActiveMesh() const;
		const Material*	const				getActiveMaterial() const;
		void								resetActives();


		bool								loadShader(Shader* s);
		void								loadMesh(Mesh* m);
		void								updateMesh(Mesh* m);
		void								loadTexture(Texture* t);
		void								loadFontBitmapTexture(FontBitmap* fb);

		RenderTarget						createRenderTarget(const std::string& name, unsigned int width, unsigned int height);
		bool								updateRenderTarget(RenderTarget* rt);
		void								clearRenderTarget(RenderTarget* rt);

		void								setActiveMaterial(Material* m);
		void								useShader(Shader* s);
		void								useMesh(Mesh* m);

		void								setShaderBool(const std::string& name, bool value);
		void								setShaderInt(const std::string& name, int value);
		void								setShaderUInt(const std::string& name, uint64_t value);
		void								setShaderFloat(const std::string& name, float value);
		void								setShaderFloatArray(const std::string& name, const std::vector<float>& value);
		void								setShaderMat4(const std::string& name, const glm::mat4& value);
		void								setShaderVec3(const std::string& name, const glm::vec3& value);
		void								setShaderVec3Array(const std::string& name, const std::vector<glm::vec3>& value);
		void								setShaderVec4(const std::string& name, const glm::vec4& value);

		void								drawGpuMesh();
		void								clearDepthBuffer();

		void								finish();
		void								enableBlend();
        void                                disableBlend();

		void								debugDrawCollisionWorld(CollisionDebugDrawer* cdd);

		void								clearShader(Shader* s);
		void								clearMesh(GpuGeoPool ggp);
		void								clearTexture(Texture* t);

		void								updateBonesUBO(const std::vector<std::pair<unsigned int, glm::mat4>>& boneData); // first = bone array index, second = bone matrix

		void								enableBackfaceCulling();
		void								disableBackfaceCulling();

		void								updateTextureUBO(unsigned int index, GLuint64 dsaHandle);
		void								updateLightmapTextureUBO(GLuint64 dsaHandle);

		void								updateCameraViewportSize(unsigned int width, unsigned int height);
		std::unique_ptr<FinalRenderTarget>	updateFinalRenderTargetVPSize(FinalRenderTarget* frt, unsigned int width, unsigned int height);
		void								setRenderTarget(RenderTarget* rt);

		void								drawToFinalRenderTarget(GLuint64 dsaHandle);

		void								disableDepthMask();
		void								enableDepthMask();



		void								setOpaqueRenderState();
		void								setAlphaRenderState();
		void								setCompositeRenderState();
		void								composeFBOs();
		void								setDefaultFrameBuffer();

		void								clearRenderTargetBuffers(float r, float g, float b, float a);
		void								clearScreenBuffer(float r, float g, float b, float a);

		void								setGLDebugMessage(const std::string& message);

		glm::ivec2							getActiveCameraViewportSize();

		void								drawLines(unsigned int pointCount);

											// adjust x,y,z as r,g,b for any color, adjust w as strength of the overlay tint
		void								drawToScreen(FinalRenderTarget* frt, glm::vec4 tint = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));

		void								clearFinalRenderTarget(FinalRenderTarget* frt, glm::vec4 color);

		void								setFinalRenderTarget(FinalRenderTarget* frt);

		void								setViewportSize(unsigned int width, unsigned int height);

		std::unique_ptr<FinalRenderTarget>	createFinalRenderTarget(const std::string& name, unsigned int width, unsigned int height);
		void								freeFinalRenderTarget(FinalRenderTarget* frt);

		std::unique_ptr<Texture>			generateEmptyTexture(const std::string& name, unsigned int frameCount, unsigned int width, unsigned int height, int options = 0);
		void								copyGPUTexture(unsigned int sourceId, unsigned int destinationId, unsigned int width, unsigned int height);

		void								fenceAndFlush();
		void								clientWaitSync();

	};
}