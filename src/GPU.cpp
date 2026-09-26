#include <fstream>
#include <sstream>
#include <filesystem>

#include <spdlog/spdlog.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stb_headers/stb_image.h>

#include <vel/GPU.h>
#include <vel/Util/functions.h>
#include <vel/Util/Assert.h>
#include <vel/Scene/ActorGpuData.h>
#include <vel/Scene/MaterialGpuData.h>




namespace vel
{
	GPU::GPU() :
		zeroFillerVec(glm::vec4(0.0f)),
		oneFillerVec(1.0f),
		activeClearColorValues(glm::vec4(0.0f)),

		screenSpaceMesh(std::make_unique<Mesh>("screenSpaceMesh")),
		screenSpaceMeshGeoPool(std::make_unique<GeoPoolT<VtxPosNrmlTx>>()),
		screenShader({.programId = 0}),
		postShader({ .programId = 0 }),
		compositeShader({ .programId = 0 }),
		activeViewportSize(glm::ivec2(-1, -1)), // previously defaulted to 1280 x 720
		prevFrameFence(0),

		bonesUBO(0),
		texturesUBO(0),
		lightmapTextureUBO(0),
		
		activeFramebuffer(-1)
	{
		this->enableBackfaceCulling();

		this->initBoneUBO();
		this->initTextureUBO();
		this->initLightMapTextureUBO();

		this->initScreenSpaceMesh();

		this->initShaders();
	}

	GPU::~GPU()
	{
		this->clearGeoPool(this->screenSpaceMeshGeoPool->gpuGeoPool.value());

		glDeleteBuffers(1, &this->bonesUBO);
		glDeleteBuffers(1, &this->texturesUBO);
		glDeleteBuffers(1, &this->lightmapTextureUBO);

		this->bonesUBO = 0;
		this->texturesUBO = 0;
		this->lightmapTextureUBO = 0;

		this->clearShader(this->screenShader.programId);
		this->clearShader(this->postShader.programId);
		this->clearShader(this->compositeShader.programId);
	}

	FinalRenderTarget GPU::createFinalRenderTarget(unsigned int width, unsigned int height)
	{
		FinalRenderTarget frt;
		frt.resolution = glm::ivec2(width, height);

		unsigned int fboId = 0;
		glGenFramebuffers(1, &fboId);
		frt.fbo = fboId;

		glGenTextures(1, &frt.colorBufferId);
		glGenTextures(1, &frt.depthBufferId);

		glBindTexture(GL_TEXTURE_2D, frt.colorBufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, frt.depthBufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0); // be safe
		
		this->bindFrameBuffer(frt.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frt.colorBufferId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frt.depthBufferId, 0);

		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);

		// verify success
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			
			std::string statusString = "";
			switch (status)
			{
			case GL_FRAMEBUFFER_UNDEFINED:
				statusString = "GL_FRAMEBUFFER_UNDEFINED";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				statusString = "GL_FRAMEBUFFER_UNSUPPORTED";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				statusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
				break;
			default:
				statusString = "Unknown error";
			}

			SPDLOG_ERROR("GPU::createFinalRenderTarget: Framebuffer is not complete! Status code: {}", statusString);
		}

		this->bindFrameBuffer(0); // be safe

		frt.colorDsaHandle = glGetTextureHandleARB(frt.colorBufferId);
		glMakeTextureHandleResidentARB(frt.colorDsaHandle);

		return frt;
	}

	void GPU::freeFinalRenderTarget(FinalRenderTarget& frt)
	{
		glMakeTextureHandleNonResidentARB(frt.colorDsaHandle);
		glDeleteTextures(1, &frt.colorBufferId);
		glDeleteTextures(1, &frt.depthBufferId);
		glDeleteFramebuffers(1, &frt.fbo);
	}

	void GPU::setOpaqueRenderState(RenderTarget& rt)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		this->bindFrameBuffer(rt.opaqueFBO);	
		this->setViewportSize(rt.resolution.x, rt.resolution.y);
	}

	void GPU::setTransparentRenderState(RenderTarget& rt)
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		glBlendEquation(GL_FUNC_ADD);

		this->bindFrameBuffer(rt.alphaFBO);
		this->setViewportSize(rt.resolution.x, rt.resolution.y);
	}

	//void GPU::setCompositeRenderState()
	//{
	//	glDepthFunc(GL_ALWAYS);
	//	glEnable(GL_BLEND);
	//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//	this->bindFrameBuffer(this->activeRenderTarget->opaqueFBO);
	//}

	// Changed above to this when troubleshooting why the edges of text that have alpha
	// were ignoring the element behind them when blending, and blending with the contents
	// of the final render target
	void GPU::setCompositeRenderState(RenderTarget& rt)
	{
		glDepthFunc(GL_ALWAYS);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		this->bindFrameBuffer(rt.opaqueFBO);
		this->setViewportSize(rt.resolution.x, rt.resolution.y);
	}

	void GPU::composeFBOs(RenderTarget& rt)
	{
		this->setCompositeRenderState(rt);
		
		this->useShader(this->compositeShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rt.accumBufferId);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, rt.revealBufferId);
		
		this->useVao(this->screenSpaceMesh->gp->gpuGeoPool->VAO);

		glDrawElements(GL_TRIANGLES, this->screenSpaceMesh->gp->indices.size(), GL_UNSIGNED_INT, 0);
	}

	void GPU::setGLDebugMessage(const std::string& message)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			GL_DEBUG_SEVERITY_NOTIFICATION, -1, message.c_str());
	}

	void GPU::drawToFinalRenderTarget(GLuint64 dsaHandle)
	{
		this->useShader(this->screenShader);

		this->updateTextureUBO(0, dsaHandle);

		this->useVao(this->screenSpaceMesh->gp->gpuGeoPool->VAO);

		glDrawElements(GL_TRIANGLES, this->screenSpaceMesh->gp->indices.size(), GL_UNSIGNED_INT, 0);
	}

	void GPU::drawToScreen(FinalRenderTarget& frt)
	{
		this->clearScreenBuffer(0.0f, 1.0f, 0.0f, 1.0f);

		this->disableBlend();

		this->useShader(this->postShader);

		glUniform1ui64ARB(this->postShaderTextureLocation, frt.colorDsaHandle);
		glUniform4fv(this->postShaderColorLocation, 1, &frt.colorMultiplier[0]);
		
		this->useVao(this->screenSpaceMesh->gp->gpuGeoPool->VAO);

		glDrawElements(GL_TRIANGLES, this->screenSpaceMesh->gp->indices.size(), GL_UNSIGNED_INT, 0);

		this->enableBlend();
	}

	void GPU::setViewportSize(unsigned int width, unsigned int height)
	{
		glm::ivec2 viewportSize(width, height);

		if (viewportSize == this->activeViewportSize)
			return;

		glViewport(0, 0, width, height);
		this->activeViewportSize = viewportSize;
	}

	std::optional<FinalRenderTarget> GPU::updateFinalRenderTargetVPSize(FinalRenderTarget& frt, unsigned int width, unsigned int height)
	{
		if (width == frt.resolution.x && height == frt.resolution.y)
			return std::nullopt;

		frt.resolution = glm::ivec2(width, height);

		if (frt.resolution.x != 0 && frt.resolution.y != 0)
		{
			this->freeFinalRenderTarget(frt);
			return this->createFinalRenderTarget(width, height);
		}

		return std::nullopt;
	}

	void GPU::setFinalRenderTarget(FinalRenderTarget& frt)
	{
		glDepthMask(GL_TRUE); // insure we're writing to depth buffer (without this, we had to have two stages each with a camera for rendering to work right, so i must be disabling it somewhere.

		this->bindFrameBuffer(frt.fbo);
		this->setViewportSize(frt.resolution.x, frt.resolution.y);
	}

	void GPU::setDefaultFrameBuffer(unsigned int width, unsigned int height)
	{
		this->bindFrameBuffer(0);
		this->setViewportSize(width, height);
	}

	void GPU::initScreenSpaceMesh()
	{
		this->screenSpaceMesh->gp = this->screenSpaceMeshGeoPool.get();
		this->screenSpaceMesh->firstIndex = this->screenSpaceMeshGeoPool->indices.size();
		this->screenSpaceMesh->baseVertex = this->screenSpaceMeshGeoPool->vertexCount();
		this->screenSpaceMesh->flags = MESHFLAG_RENDERABLE;


		// top left
		VtxPosNrmlTx v0;
		v0.position = glm::vec3(-1.0f, 1.0f, 0.0f);
		v0.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v0.textureCoords = glm::vec2(0.0f, 1.0f);
		this->screenSpaceMeshGeoPool->vertices.push_back(v0);

		// bottom left
		VtxPosNrmlTx v1;
		v1.position = glm::vec3(-1.0f, -1.0f, 0.0f);
		v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v1.textureCoords = glm::vec2(0.0f, 0.0f);
		this->screenSpaceMeshGeoPool->vertices.push_back(v1);

		// bottom right
		VtxPosNrmlTx v2;
		v2.position = glm::vec3(1.0f, -1.0f, 0.0f);
		v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v2.textureCoords = glm::vec2(1.0f, 0.0f);
		this->screenSpaceMeshGeoPool->vertices.push_back(v2);

		// top right
		VtxPosNrmlTx v3;
		v3.position = glm::vec3(1.0f, 1.0f, 0.0f);
		v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v3.textureCoords = glm::vec2(1.0f, 1.0f);
		this->screenSpaceMeshGeoPool->vertices.push_back(v3);

		std::vector<unsigned int> is = { 0,1,2,0,2,3 };

		this->screenSpaceMeshGeoPool->indices.push_back(0);
		this->screenSpaceMeshGeoPool->indices.push_back(1);
		this->screenSpaceMeshGeoPool->indices.push_back(2);
		this->screenSpaceMeshGeoPool->indices.push_back(0);
		this->screenSpaceMeshGeoPool->indices.push_back(2);
		this->screenSpaceMeshGeoPool->indices.push_back(3);

		this->screenSpaceMesh->indexCount = this->screenSpaceMesh->gp->indices.size() - this->screenSpaceMesh->firstIndex;

		this->screenSpaceMesh->refreshAABB();

		this->loadGeoPool(this->screenSpaceMeshGeoPool.get());
	}

	void GPU::enableBackfaceCulling()
	{
		glEnable(GL_CULL_FACE);
	}

	void GPU::disableBackfaceCulling()
	{
		glDisable(GL_CULL_FACE);
	}

	void GPU::initLightMapTextureUBO()
	{
		glGenBuffers(1, &this->lightmapTextureUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, this->lightmapTextureUBO);
		//glBufferData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * 2, NULL, GL_STATIC_DRAW);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * 2, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, this->lightmapTextureUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::updateLightmapTextureUBO(GLuint64 dsaHandle)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->lightmapTextureUBO);
		//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GLuint64) * 2, (void*)&dsaHandle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GLuint64), (void*)&dsaHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}


	void GPU::initTextureUBO()
	{
		const int MAX_SUPPORTED_TEXTURES = 250;
		glGenBuffers(1, &this->texturesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, this->texturesUBO);
		//glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_TEXTURES * sizeof(GLuint64) * 2, NULL, GL_STATIC_DRAW);
		glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_TEXTURES * sizeof(GLuint64) * 2, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->texturesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::updateTextureUBO(unsigned int index, GLuint64 dsaHandle)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->texturesUBO);
		//glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * index * 2, sizeof(GLuint64) * 2, (void*)&dsaHandle);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * index * 2, sizeof(GLuint64), (void*)&dsaHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}


	void GPU::initBoneUBO()
	{
		const int MAX_SUPPORTED_BONES = 200;
		glGenBuffers(1, &this->bonesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, this->bonesUBO);
		//glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_BONES * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
		glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_BONES * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, this->bonesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::updateBonesUBO(const std::vector<std::pair<unsigned int, glm::mat4>>& boneData)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->bonesUBO);

		for (auto& bd : boneData)
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * bd.first, sizeof(glm::mat4), glm::value_ptr(bd.second));

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}


	void GPU::clearShader(unsigned int programId)
	{
		glDeleteProgram(programId);
	}

	void GPU::clearGeoPool(GpuGeoPool ggp)
	{
		glDeleteVertexArrays(1, &ggp.VAO);
		glDeleteBuffers(1, &ggp.VBO);
		glDeleteBuffers(1, &ggp.EBO);
	}

	void GPU::clearTexture(Texture& t)
	{
		if (t.dsaHandle != 0)
			glMakeTextureHandleNonResidentARB(t.dsaHandle);

		if (t.bufferId != 0)
			glDeleteTextures(1, &t.bufferId);

		t.dsaHandle = 0;
		t.bufferId = 0;
	}

	void GPU::clearRenderTarget(RenderTarget& rt)
	{
		if (rt.opaqueDsaHandle != 0)
			glMakeTextureHandleNonResidentARB(rt.opaqueDsaHandle);

		if (rt.depthDsaHandle != 0)
			glMakeTextureHandleNonResidentARB(rt.depthDsaHandle);

		if (rt.accumDsaHandle != 0)
			glMakeTextureHandleNonResidentARB(rt.accumDsaHandle);

		if (rt.revealDsaHandle != 0)
			glMakeTextureHandleNonResidentARB(rt.revealDsaHandle);

		glDeleteTextures(1, &rt.opaqueBufferId);
		glDeleteTextures(1, &rt.depthBufferId);
		glDeleteTextures(1, &rt.accumBufferId);
		glDeleteTextures(1, &rt.revealBufferId);

		glDeleteFramebuffers(1, &rt.opaqueFBO);
		glDeleteFramebuffers(1, &rt.alphaFBO);

		rt = {};
	}

	bool GPU::loadShader(Shader& s, const std::string& vertCode, const std::string& fragCode)
	{
		int success;
		char infoLog[512];
		std::string infoLogStr = "";

		/////////////////////////////////////////////////////////
		// Vertex Shader
		/////////////////////////////////////////////////////////
		const char* vShaderCode = vertCode.c_str();
		unsigned int vertex = 0;
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		// if compile errors, log and exit
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: VERTEX::COMPILATION_FAILED: {}", infoLogStr);
			return false;
		};

		/////////////////////////////////////////////////////////
		// Fragment Shader
		/////////////////////////////////////////////////////////
		const char* fShaderCode = fragCode.c_str();
		unsigned int fragment = 0;
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		// if compile errors, log and exit
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: FRAGMENT::COMPILATION_FAILED: {}", infoLogStr);
			return false;
		};


		/////////////////////////////////////////////////////////
		// Shader Program
		/////////////////////////////////////////////////////////
		unsigned int id = 0;
		id = glCreateProgram();
		glAttachShader(id, vertex);
		glAttachShader(id, fragment);
		glLinkProgram(id);

		// if linking errors, log and exit
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: PROGRAM::LINKING_FAILED: {}", infoLog);
			return false;
		}

		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragment);

		s.programId = id;

		return true;
	}

	bool GPU::loadShader(Shader& s, const std::string& vertCode, const std::string& geomCode, const std::string& fragCode)
	{
		int success;
		char infoLog[512];
		std::string infoLogStr = "";

		/////////////////////////////////////////////////////////
		// Vertex Shader
		/////////////////////////////////////////////////////////
		const char* vShaderCode = vertCode.c_str();
		unsigned int vertex = 0;
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		// if compile errors, log and exit
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: VERTEX::COMPILATION_FAILED: {}", infoLogStr);
			return false;
		};

		/////////////////////////////////////////////////////////
		// Geometry Shader
		/////////////////////////////////////////////////////////
		const char* gShaderCode = geomCode.c_str();
		unsigned int geometry = 0;
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);

		// if compile errors, log and exit
		glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(geometry, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: GEOMETRY::COMPILATION_FAILED: {}", infoLogStr);
			return false;
		};

		/////////////////////////////////////////////////////////
		// Fragment Shader
		/////////////////////////////////////////////////////////
		const char* fShaderCode = fragCode.c_str();
		unsigned int fragment = 0;
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		// if compile errors, log and exit
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: FRAGMENT::COMPILATION_FAILED: {}", infoLogStr);
			return false;
		};


		/////////////////////////////////////////////////////////
		// Shader Program
		/////////////////////////////////////////////////////////
		unsigned int id = 0;
		id = glCreateProgram();
		glAttachShader(id, vertex);
		if (geometry > 0)
			glAttachShader(id, geometry);
		glAttachShader(id, fragment);
		glLinkProgram(id);

		// if linking errors, log and exit
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			infoLogStr = infoLog;

			SPDLOG_DEBUG("GPU::loadShader: PROGRAM::LINKING_FAILED: {}", infoLog);
			return false;
		}

		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(geometry);
		glDeleteShader(fragment);

		s.programId = id;

		return true;
	}

	RenderTarget GPU::createRenderTarget(unsigned int width, unsigned int height)
	{
		RenderTarget rt;
		rt.resolution = glm::ivec2(width, height);

		glGenFramebuffers(1, &rt.opaqueFBO);
		glGenFramebuffers(1, &rt.alphaFBO);

		glGenTextures(1, &rt.opaqueBufferId);
		glGenTextures(1, &rt.depthBufferId);
		glGenTextures(1, &rt.accumBufferId);
		glGenTextures(1, &rt.revealBufferId);
		
		this->updateRenderTarget(rt);

		rt.opaqueDsaHandle = glGetTextureHandleARB(rt.opaqueBufferId);
		rt.depthDsaHandle = glGetTextureHandleARB(rt.depthBufferId);
		rt.accumDsaHandle = glGetTextureHandleARB(rt.accumBufferId);
		rt.revealDsaHandle = glGetTextureHandleARB(rt.revealBufferId);

		glMakeTextureHandleResidentARB(rt.opaqueDsaHandle);
		glMakeTextureHandleResidentARB(rt.depthDsaHandle);
		glMakeTextureHandleResidentARB(rt.accumDsaHandle);
		glMakeTextureHandleResidentARB(rt.revealDsaHandle);

		return rt;
	}

	bool GPU::updateRenderTarget(RenderTarget& rt)
	{
		if (rt.resolution.x == 0 || rt.resolution.y == 0)
			return false;

		//
		// Configure opaqueFBO
		//

		// opaque texture
		glBindTexture(GL_TEXTURE_2D, rt.opaqueBufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rt.resolution.x, rt.resolution.y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// depth texture
		glBindTexture(GL_TEXTURE_2D, rt.depthBufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, rt.resolution.x, rt.resolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		// unbind texture (not necessary, but we do it anyway)
		glBindTexture(GL_TEXTURE_2D, 0);

		// associate textures with opaqueFBO
		this->bindFrameBuffer(rt.opaqueFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.opaqueBufferId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt.depthBufferId, 0);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			SPDLOG_DEBUG("GPU::updateRenderTarget(): Framebuffer is not complete: 001");
			return false;
		}

		this->bindFrameBuffer(0);


		//
		// Configure alphaFBO
		//

		// accum texture
		glBindTexture(GL_TEXTURE_2D, rt.accumBufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rt.resolution.x, rt.resolution.y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// reveal texture
		glBindTexture(GL_TEXTURE_2D, rt.revealBufferId);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rt->resolution.x, rt->resolution.y, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rt.resolution.x, rt.resolution.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// unbind texture (not necessary, but we do it anyway)
		glBindTexture(GL_TEXTURE_2D, 0);

		// associate textures with alphaFBO
		this->bindFrameBuffer(rt.alphaFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.accumBufferId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt.revealBufferId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt.depthBufferId, 0);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			SPDLOG_DEBUG("GPU::updateRenderTarget(): Framebuffer is not complete: 002");
			return false;
		}

		const GLenum transparentDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, transparentDrawBuffers);



		this->bindFrameBuffer(0);

		return true;
	}

	void GPU::genVtxPosBuffer(GeoPoolT<VtxPos>* gp)
	{
		GpuGeoPool ggp = GpuGeoPool();

		glGenVertexArrays(1, &ggp.VAO);
		glBindVertexArray(ggp.VAO);

		glGenBuffers(1, &ggp.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPos), gp->vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPos), (void*)0);

		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::genVtxPosNrmlBuffer(GeoPoolT<VtxPosNrml>* gp)
	{
		GpuGeoPool ggp = GpuGeoPool();

		glGenVertexArrays(1, &ggp.VAO);
		glBindVertexArray(ggp.VAO);

		glGenBuffers(1, &ggp.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrml), gp->vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrml), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrml), (void*)offsetof(VtxPosNrml, normal));

		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::genVtxPosNrmlTxBuffer(GeoPoolT<VtxPosNrmlTx>* gp)
	{
		GpuGeoPool ggp = GpuGeoPool();

		glGenVertexArrays(1, &ggp.VAO);
		glBindVertexArray(ggp.VAO);

		glGenBuffers(1, &ggp.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTx), gp->vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)offsetof(VtxPosNrmlTx, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)offsetof(VtxPosNrmlTx, textureCoords));

		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::genVtxPosNrmlTxLmBuffer(GeoPoolT<VtxPosNrmlTxLm>* gp)
	{
		GpuGeoPool ggp = GpuGeoPool();

		glGenVertexArrays(1, &ggp.VAO);
		glBindVertexArray(ggp.VAO);

		glGenBuffers(1, &ggp.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxLm), gp->vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, textureCoords));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, lightmapCoords));


		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::genVtxPosNrmlTxSknBuffer(GeoPoolT<VtxPosNrmlTxSkn>* gp)
	{
		GpuGeoPool ggp = GpuGeoPool();

		// Generate and bind vertex attribute array
		glGenVertexArrays(1, &ggp.VAO);
		glBindVertexArray(ggp.VAO);

		// Generate and bind vertex buffer object
		glGenBuffers(1, &ggp.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxSkn), gp->vertices.data(), GL_STATIC_DRAW);

		// Generate and bind element buffer object
		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		// Assign vertex positions to location = 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)0);

		// Assign vertex normals to location = 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, normal));

		// Assign vertex texture coordinates to location = 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, textureCoords));

		// Assign vertex bone ids to location = 4
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, boneIds));

		// Assign vertex weights to location = 5
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, boneWeights));

		// Unbind the vertex array to prevent accidental operations
		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::loadGeoPool(GeoPool* gp)
	{
		VEL_ASSERT(gp, "GPU::loadGeoPool(): Null geometry pool.");
		VEL_ASSERT(!gp->gpuGeoPool, "GPU::loadGeoPool(): Geometry pool already initialized.");

		switch (gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			this->genVtxPosBuffer(static_cast<GeoPoolT<VtxPos>*>(gp));
			break;
		case VtxLayout::VTX_POS_NRML:
			this->genVtxPosNrmlBuffer(static_cast<GeoPoolT<VtxPosNrml>*>(gp));
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			this->genVtxPosNrmlTxBuffer(static_cast<GeoPoolT<VtxPosNrmlTx>*>(gp));
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			this->genVtxPosNrmlTxLmBuffer(static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(gp));
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			this->genVtxPosNrmlTxSknBuffer(static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(gp));
			break;
		}

		VEL_ASSERT(gp->gpuGeoPool.has_value(), "GPU::loadGeoPool(): Failed to initialize GPU geometry pool.");
	}

	void GPU::updateGeoPool(GeoPool* gp)
	{
		VEL_ASSERT(gp && gp->gpuGeoPool, "GPU::updateGeoPool(): Geometry pool has not been initialized.");

		GpuGeoPool& ggp = gp->gpuGeoPool.value();

		glBindVertexArray(ggp.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);

		switch (gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPos), static_cast<GeoPoolT<VtxPos>*>(gp)->vertices.data(), GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrml), static_cast<GeoPoolT<VtxPosNrml>*>(gp)->vertices.data(), GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTx), static_cast<GeoPoolT<VtxPosNrmlTx>*>(gp)->vertices.data(), GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxLm), static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(gp)->vertices.data(), GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxSkn), static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(gp)->vertices.data(), GL_STATIC_DRAW);
			break;
		}

		// Bind and update indices buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), gp->indices.data(), GL_STATIC_DRAW);

		// Unbind the vertex array to prevent accidental operations
		glBindVertexArray(0);
	}

	void GPU::copyGPUTexture(unsigned int sourceId, unsigned int destinationId, unsigned int width, unsigned int height)
	{
		glCopyImageSubData(
			sourceId, GL_TEXTURE_2D, 0, 0, 0, 0,
			destinationId, GL_TEXTURE_2D, 0, 0, 0, 0,
			width, height, 1
		);
	}

	Texture GPU::generateEmptyTexture(unsigned int width, unsigned int height, int flags)
	{
		Texture t;
		t.flags = flags;

		glGenTextures(1, &t.bufferId);
		glBindTexture(GL_TEXTURE_2D, t.bufferId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (t.flags & TXTRFLG_CLAMP_UVS)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		t.dsaHandle = glGetTextureHandleARB(t.bufferId);
		glMakeTextureHandleResidentARB(t.dsaHandle);		

		return t;
	}

	void GPU::loadTexture(Texture& t)
	{
		switch (t.channels)
		{
		case 1:
			t.sizedFormat = GL_R8; // 8 bits per channel x1 channel
			t.format = GL_RED;
			break;
		case 2:
			t.sizedFormat = GL_RG8; // 8 bits per channel x2 channels
			t.format = GL_RG;
			break;
		case 3:
			t.sizedFormat = GL_RGB8; // 8 bits per channel x3 channels
			t.format = GL_RGB;
			break;
		case 4:
			t.sizedFormat = GL_RGBA8; // 8 bits per channel x4 channels
			t.format = GL_RGBA;
			break;
		}

		// create a texture buffer and bind it to context
		glGenTextures(1, &t.bufferId);
		glBindTexture(GL_TEXTURE_2D, t.bufferId);

		// load data into the buffer
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			t.sizedFormat,
			t.width,
			t.height,
			0,
			t.format,
			GL_UNSIGNED_BYTE,
			t.data
		);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // reset default state

		//// auto generate mipmap levels for texture
		glGenerateMipmap(GL_TEXTURE_2D);

		// set texture parameters
		if (t.flags & TXTRFLG_CLAMP_UVS)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
			
		if (t.flags & TXTRFLG_FILTER)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			
		}

		// obtain texture's DSA handle
		t.dsaHandle = glGetTextureHandleARB(t.bufferId);

		// set texture's DSA handle as resident so it can be accessed in shaders
		glMakeTextureHandleResidentARB(t.dsaHandle);

		if(!(t.flags & TXTRFLG_CPU_AND_GPU))
			stbi_image_free(t.data);
	}

	Texture GPU::generateFontBitmapTexture(FontBitmap* fb)
	{
		Texture t;

		glGenTextures(1, &t.bufferId);
		glBindTexture(GL_TEXTURE_2D, t.bufferId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_R8,
			fb->textureWidth,
			fb->textureHeight,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			fb->data.get()
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // reset pack alignment to default


		t.dsaHandle = glGetTextureHandleARB(t.bufferId);
		glMakeTextureHandleResidentARB(t.dsaHandle);

		return t;
	}

	void GPU::useShader(Shader s)
	{
		glUseProgram(s.programId);
	}

	void GPU::useVao(unsigned int vao)
	{
		glBindVertexArray(vao);
	}

	void GPU::disableDepthMask()
	{
		glDepthMask(GL_FALSE);
	}

	void GPU::enableDepthMask()
	{
		glDepthMask(GL_TRUE);
	}

	void GPU::enableDepthTest()
	{
		glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
	}
    
    void GPU::enableBlend()
	{
		glEnable(GL_BLEND);
	}
    
    void GPU::disableBlend()
    {
        glDisable(GL_BLEND);
    }

	void GPU::clearDepthBuffer()
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void GPU::clearBuffers(float r, float g, float b, float a)
	{
		if (r != this->activeClearColorValues.x || g != this->activeClearColorValues.y ||
			b != this->activeClearColorValues.z || a != this->activeClearColorValues.w)
		{
			this->activeClearColorValues = glm::vec4(r,g,b,a);
			glClearColor(r, g, b, a);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GPU::clearRenderTargetBuffers(RenderTarget& rt, float r, float g, float b, float a)
	{
		this->bindFrameBuffer(rt.opaqueFBO);
		this->clearBuffers(r,g,b,a);

		this->bindFrameBuffer(rt.alphaFBO);
		glClearBufferfv(GL_COLOR, 0, &this->zeroFillerVec[0]);
		glClearBufferfv(GL_COLOR, 1, &this->oneFillerVec[0]);
	}

	void GPU::clearFinalRenderTarget(FinalRenderTarget& frt, glm::vec4 color)
	{
		this->bindFrameBuffer(frt.fbo);
		this->clearBuffers(color.x, color.y, color.z, color.w);
	}

	void GPU::clearScreenBuffer(float r, float g, float b, float a)
	{
		this->bindFrameBuffer(0);
		this->clearBuffers(r, g, b, a);
	}

	void GPU::bindFrameBuffer(unsigned int fbo)
	{
		if (this->activeFramebuffer != fbo)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			this->activeFramebuffer = fbo;
		}
	}

	void GPU::drawLinesOnly()
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	void GPU::finish()
	{
		glFinish();
	}

	void GPU::fenceAndFlush()
	{
		this->prevFrameFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush(); // ensure fence + commands are in the GPU queue
	}

	void GPU::clientWaitSync()
	{
		if (this->prevFrameFence)
		{
			glClientWaitSync(this->prevFrameFence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			glDeleteSync(this->prevFrameFence);
			this->prevFrameFence = 0;
		}
	}

	void GPU::drawLines(unsigned int pointCount)
	{
		glDrawArrays(GL_LINES, 0, pointCount);
	}

	void GPU::debugDrawCollisionWorld(CollisionDebugDrawer* cdd)
	{
		if (cdd->getVerts().size() > 0)
		{
			unsigned int VAO, VBO;
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			glBufferData(GL_ARRAY_BUFFER, cdd->getVerts().size() * sizeof(BulletDebugDrawData), &cdd->getVerts()[0], GL_STATIC_DRAW);

			// Assign vertex positions to location = 0
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BulletDebugDrawData), (void*)0);

			// Assign vertex color to location = 1
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BulletDebugDrawData), (void*)offsetof(BulletDebugDrawData, color));

			//glDrawArrays(GL_LINES, 0, (GLsizei)this->verts.size() / 3);
			glDrawArrays(GL_LINES, 0, (GLsizei)cdd->getVerts().size());

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			cdd->getVerts().clear();

			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		}
	}

	void GPU::initShaders()
	{
		////////////////////////////////////////////////////////
		// Screen Shader
		////////////////////////////////////////////////////////
		std::string vertCode = R"GLSL(
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aLightMapCoords;
layout (location = 4) in uint aTexId;

out vec2 TexCoords;
flat out uint TexId;

void main()
{
    TexCoords = aTexCoords;
	TexId = aTexId;

	gl_Position = vec4(aPos, 1.0);
}
)GLSL";

		std::string fragCode = R"GLSL(
#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

in vec2 TexCoords;
in vec2 LMTexCoords;
flat in uint TexId;

const int MAX_TEXTURE_SLOTS = 250;
layout (std140, binding = 0) uniform TexturesUBO
{
    sampler2D tex[MAX_TEXTURE_SLOTS];
};

out vec4 FragColor;

void main()
{	
	FragColor = texture(tex[TexId], TexCoords).rgba;
}
)GLSL";

		this->loadShader(this->screenShader, vertCode, fragCode);


		////////////////////////////////////////////////////////
		// Post Shader
		////////////////////////////////////////////////////////

		vertCode = R"GLSL(
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
flat out uint TexId;

void main()
{
    TexCoords = aTexCoords;

	gl_Position = vec4(aPos, 1.0);
}
)GLSL";

		fragCode = R"GLSL(
#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

in vec2 TexCoords;

uniform vec4 color;
uniform uint64_t textureHandle;

out vec4 FragColor;

void main()
{	
	sampler2D tex = sampler2D(textureHandle);
	vec4 texColor = texture(tex, TexCoords);
	FragColor = mix(texColor, vec4(color.rgb, 1.0), color.a);
	//FragColor = texColor * color;
}
)GLSL";

		this->loadShader(this->postShader, vertCode, fragCode);

		this->postShaderColorLocation = glGetUniformLocation(this->postShader.programId, "color");
		this->postShaderTextureLocation = glGetUniformLocation(this->postShader.programId, "textureHandle");


		////////////////////////////////////////////////////////
		// Composite Shader
		////////////////////////////////////////////////////////
		vertCode = R"GLSL(
#version 460 core

layout (location = 0) in vec3 position;

void main()
{
	gl_Position = vec4(position, 1.0f);
}
)GLSL";

		fragCode = R"GLSL(
#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

// shader outputs
layout (location = 0) out vec4 frag;

// color accumulation buffer
layout (binding = 0) uniform sampler2D accum;

// revealage threshold buffer
layout (binding = 1) uniform sampler2D reveal;

// epsilon number
const float EPSILON = 0.00001f;

// calculate floating point numbers equality accurately
bool isApproximatelyEqual(float a, float b)
{
	return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

// get the max value between three values
float max3(vec3 v) 
{
	return max(max(v.x, v.y), v.z);
}

void main()
{
	// fragment coordination
	ivec2 coords = ivec2(gl_FragCoord.xy);
	
	// fragment revealage
	float revealage = texelFetch(reveal, coords, 0).r;
	
	// save the blending and color texture fetch cost if there is not a transparent fragment
	if (isApproximatelyEqual(revealage, 1.0f)) 
		discard;
 
	// fragment color
	vec4 accumulation = texelFetch(accum, coords, 0);
	
	// suppress overflow
	if (isinf(max3(abs(accumulation.rgb)))) 
		accumulation.rgb = vec3(accumulation.a);

	// prevent floating point precision bug
	vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

	// blend pixels
	frag = vec4(average_color, 1.0f - revealage);
}
)GLSL";

		this->loadShader(this->compositeShader, vertCode, fragCode);

	}

	///////////////////////////////////////////
	// New stuff
	///////////////////////////////////////////
	void GPU::createBuffer(uint32_t* id)
	{
		glCreateBuffers(1, id);
	}

	void GPU::deleteBuffer(uint32_t* id)
	{
		glDeleteBuffers(1, id);
	}

	void GPU::uploadStaticBufferData(uint32_t id, uint32_t size, void* data)
	{
		glNamedBufferData(id, size, data, GL_STATIC_DRAW);
	}

	void GPU::uploadStreamBufferData(uint32_t id, uint32_t size, void* data)
	{
		glNamedBufferData(id, size, data, GL_STREAM_DRAW);
	}

	void GPU::uploadBufferSubData(uint32_t buffer, uint32_t offset, uint32_t size, void* data)
	{
		glNamedBufferSubData(buffer, offset, size, data);
	}

	void GPU::submitDrawBucket(const DrawBucket& bucket)
	{
		if (bucket.drawCommands.empty())
			return;

		glUseProgram(bucket.shader);

		glBindVertexArray(bucket.vao);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bucket.indirectBuffer); // provides both the MDI commands and the per-draw Material index.

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bucket.indirectBuffer);

		glMultiDrawElementsIndirect(
			GL_TRIANGLES,
			GL_UNSIGNED_INT,
			nullptr,
			static_cast<GLsizei>(bucket.drawCommands.size()),
			sizeof(DrawBucketCommand)
		);
	}

	void GPU::initSceneBuffers(BufferIds& b)
	{
		glCreateBuffers(1, &b.cameraUbo);
		glNamedBufferData(b.cameraUbo, sizeof(CameraGpuData), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &b.actorDataSsbo);
		glNamedBufferData(b.actorDataSsbo, sizeof(ActorGpuData), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &b.materialDataSsbo);
		glNamedBufferData(b.materialDataSsbo, sizeof(MaterialGpuData), nullptr, GL_STATIC_DRAW);

		glCreateBuffers(1, &b.materialTextureHandlesSsbo);
		glNamedBufferData(b.materialTextureHandlesSsbo, sizeof(uint64_t), nullptr, GL_STATIC_DRAW);

		glCreateBuffers(1, &b.actorAmbientCubeSsbo);
		glNamedBufferData(b.actorAmbientCubeSsbo, sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &b.actorBoneMatricesSsbo);
		glNamedBufferData(b.actorBoneMatricesSsbo, sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
	}

	void GPU::bindSceneBuffers(BufferIds& b)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, b.cameraUbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, b.actorDataSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, b.materialDataSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, b.materialTextureHandlesSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, b.actorAmbientCubeSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, b.actorBoneMatricesSsbo);
	}

	void GPU::freeSceneBuffers(BufferIds& b)
	{
		glDeleteBuffers(1, &b.cameraUbo);
		glDeleteBuffers(1, &b.actorDataSsbo);
		glDeleteBuffers(1, &b.materialDataSsbo);
		glDeleteBuffers(1, &b.materialTextureHandlesSsbo);
		glDeleteBuffers(1, &b.actorAmbientCubeSsbo);
		glDeleteBuffers(1, &b.actorBoneMatricesSsbo);
	}


} // END NAMESPACE