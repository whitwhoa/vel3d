#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <cstdint>

#include <glm/glm.hpp>

#include <vel/Scene/Stage/DrawBucket/DrawBucket.h>
#include <vel/Scene/BufferIds.h>
#include <vel/Scene/Mesh/Mesh.h>
#include <vel/Scene/Texture.h>
#include <vel/Scene/Material.h>
#include <vel/Scene/CollisionWorld/CollisionDebugDrawer.h>
#include <vel/Scene/Font/FontBitmap.h>

#include <vel/Scene/Shader.h>
#include <vel/Scene/Camera/Camera.h>
#include <vel/Scene/FinalRenderTarget.h>

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
		std::unique_ptr<GeoPoolT<VtxPosNrmlTx>>	screenSpaceMeshGeoPool;
		Shader								screenShader;

		Shader								postShader;
		int									postShaderTextureLocation;
		int									postShaderColorLocation;

		Shader								compositeShader;
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


		void								genVtxPosBuffer(GeoPoolT<VtxPos>* gp);
		void								genVtxPosNrmlBuffer(GeoPoolT<VtxPosNrml>* gp);
		void								genVtxPosNrmlTxBuffer(GeoPoolT<VtxPosNrmlTx>* gp);
		void								genVtxPosNrmlTxLmBuffer(GeoPoolT<VtxPosNrmlTxLm>* gp);
		void								genVtxPosNrmlTxSknBuffer(GeoPoolT<VtxPosNrmlTxSkn>* gp);

	public:
		GPU(bool fxaa = false);
		~GPU();
		GPU(GPU&&) = default;
        void								enableDepthTest();
		void								clearBuffers(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);
		void								drawLinesOnly();

		bool								loadShader(Shader& s, const std::string& vertCode, const std::string& fragCode);
		bool								loadShader(Shader& s, const std::string& vertCode, const std::string& geomCode, const std::string& fragCode);
		void								loadGeoPool(GeoPool* gp);
		void								updateGeoPool(GeoPool* m);
		void								loadTexture(Texture& t);
		Texture								generateFontBitmapTexture(FontBitmap* fb);

		RenderTarget						createRenderTarget(unsigned int width, unsigned int height);
		bool								updateRenderTarget(RenderTarget& rt);
		void								clearRenderTarget(RenderTarget& rt);

		void								useShader(Shader s);
		void								useVao(unsigned int vao);

		void								clearDepthBuffer();

		void								finish();
		void								enableBlend();
        void                                disableBlend();

		void								debugDrawCollisionWorld(CollisionDebugDrawer* cdd);

		void								clearShader(unsigned int programId);
		void								clearGeoPool(GpuGeoPool ggp);
		void								clearTexture(Texture& t);

		void								updateBonesUBO(const std::vector<std::pair<unsigned int, glm::mat4>>& boneData); // first = bone array index, second = bone matrix

		void								enableBackfaceCulling();
		void								disableBackfaceCulling();

		void								updateTextureUBO(unsigned int index, GLuint64 dsaHandle);
		void								updateLightmapTextureUBO(GLuint64 dsaHandle);

		void								updateCameraViewportSize(unsigned int width, unsigned int height);
		std::optional<FinalRenderTarget>	updateFinalRenderTargetVPSize(FinalRenderTarget& frt, unsigned int width, unsigned int height);

		void								drawToFinalRenderTarget(GLuint64 dsaHandle);

		void								disableDepthMask();
		void								enableDepthMask();



		void								setOpaqueRenderState(RenderTarget& rt);
		void								setTransparentRenderState(RenderTarget& rt);
		void								setCompositeRenderState(RenderTarget& rt);
		void								composeFBOs(RenderTarget& rt);
		void								setDefaultFrameBuffer();

		void								clearRenderTargetBuffers(RenderTarget& rt, float r, float g, float b, float a);
		void								clearScreenBuffer(float r, float g, float b, float a);

		void								setGLDebugMessage(const std::string& message);

		glm::ivec2							getActiveCameraViewportSize();

		void								drawLines(unsigned int pointCount);

											// adjust x,y,z as r,g,b for any color, adjust w as strength of the overlay tint
		void								drawToScreen(FinalRenderTarget& frt);

		void								clearFinalRenderTarget(FinalRenderTarget& frt, glm::vec4 color);

		void								setFinalRenderTarget(FinalRenderTarget& frt);

		void								setViewportSize(unsigned int width, unsigned int height);

		FinalRenderTarget					createFinalRenderTarget(unsigned int width, unsigned int height);
		void								freeFinalRenderTarget(FinalRenderTarget& frt);

		Texture								generateEmptyTexture(unsigned int width, unsigned int height, int flags = 0);
		void								copyGPUTexture(unsigned int sourceId, unsigned int destinationId, unsigned int width, unsigned int height);

		void								fenceAndFlush();
		void								clientWaitSync();



		////////////////////////////////////
		// New Stuff
		////////////////////////////////////
		void								createBuffer(uint32_t* id);
		void								deleteBuffer(uint32_t* id);

		void								initSceneBuffers(BufferIds& b);
		void								bindSceneBuffers(BufferIds& b);

		void								uploadStaticBufferData(uint32_t id, uint32_t size, void* data);
		void								uploadStreamBufferData(uint32_t id, uint32_t size, void* data);
		void								uploadStreamBufferSubData(uint32_t buffer, uint32_t offset, uint32_t size, void* data);
		void								submitDrawBucket(const DrawBucket& bucket);

		void								freeSceneBuffers(BufferIds& b);




	};
}