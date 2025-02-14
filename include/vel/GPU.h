#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "glm/glm.hpp"
//#include "glad/glad.h"


#include "vel/Shader.h"
#include "vel/Camera.h"
#include "vel/Mesh.h"
#include "vel/Texture.h"
#include "vel/Material.h"
#include "vel/CollisionDebugDrawer.h"
#include "vel/RenderTarget.h"
#include "vel/FontBitmap.h"

typedef uint64_t GLuint64;

namespace vel
{
	class GPU
	{
	private:
        Shader*								screenShader;
		Shader*								compositeShader;
		uint64_t							defaultWhiteTextureHandle;
		RenderTarget*						activeRenderTarget;
		Shader*								activeShader;
		Mesh*								activeMesh;
		Material*							activeMaterial;
		unsigned int						bonesUBO;
		void								initBoneUBO();

		unsigned int						texturesUBO;
		void								initTextureUBO();

		unsigned int						lightmapTextureUBO;
		void								initLightMapTextureUBO();

		Mesh								screenSpaceMesh;
		void								initScreenSpaceMesh();

		glm::vec4							zeroFillerVec;
		glm::vec4							oneFillerVec;

		glm::vec4							activeClearColorValues;
		glm::ivec2							activeViewportSize;
		int									activeFramebuffer;


	public:
		GPU();
		~GPU();
		GPU(GPU&&) = default;
        void								enableDepthTest();
		void								clearBuffers(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);
		void								drawLinesOnly();

		
		const Shader* const					getActiveShader() const;
		const Mesh*	const					getActiveMesh() const;
		const Material*	const				getActiveMaterial() const;
		void								resetActives();


		void								loadShader(Shader* s);
		void								loadMesh(Mesh* m);
		void								updateMesh(Mesh* m);
		void								loadTexture(Texture* t);
		void								loadFontBitmapTexture(FontBitmap* fb);

		RenderTarget						createRenderTarget(const std::string& name, unsigned int width, unsigned int height);
		void								updateRenderTarget(RenderTarget* rt);
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
		void								enableBlend2();
        void                                disableBlend();

		void								debugDrawCollisionWorld(CollisionDebugDrawer* cdd);

		void								clearShader(Shader* s);
		void								clearMesh(Mesh* m);
		void								clearTexture(Texture* t);

		void								updateBonesUBO(const std::vector<std::pair<unsigned int, glm::mat4>>& boneData); // first = bone array index, second = bone matrix

		void								enableBackfaceCulling();
		void								disableBackfaceCulling();

		void								updateTextureUBO(unsigned int index, GLuint64 dsaHandle);
		void								updateLightmapTextureUBO(GLuint64 dsaHandle);

		void								updateViewportSize(unsigned int width, unsigned int height);
		void								setRenderTarget(RenderTarget* rt);

		void								drawScreen(GLuint64 dsaHandle, glm::vec4 screenColor);

		void								setScreenShader(Shader* s);
		void								setCompositeShader(Shader* s);
		void								setDefaultWhiteTextureHandle(uint64_t th);

		void								disableDepthMask();
		void								enableDepthMask();



		void								setOpaqueRenderState();
		void								setAlphaRenderState();
		void								setCompositeRenderState();
		void								composeFBOs();
		void								setScreenRenderTarget();

		void								clearRenderTargetBuffers(float r, float g, float b, float a);
		void								clearScreenBuffer(float r, float g, float b, float a);

		void								setGLDebugMessage(const std::string& message);

		glm::ivec2							getActiveViewportSize();

		void								drawLines(unsigned int pointCount);

	};
}