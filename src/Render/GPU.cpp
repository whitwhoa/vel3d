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

#include <vel/Render/GPU.h>
#include <vel/Util/functions.h>




namespace vel
{
	GPU::GPU(bool fxaa) :
		zeroFillerVec(glm::vec4(0.0f)),
		oneFillerVec(1.0f),
		activeClearColorValues(glm::vec4(0.0f)),

		screenSpaceMesh(std::make_unique<Mesh>("screenSpaceMesh")),
		screenSpaceMeshGeoPool(std::make_unique<GeoPoolT<VtxPosNrmlTx>>()),
		screenShader(std::make_unique<Shader>()),
		postShader(std::make_unique<Shader>()),
		compositeShader(std::make_unique<Shader>()),
		activeRenderTarget(nullptr),
		activeShader(nullptr),
		activeMesh(nullptr),
		activeMaterial(nullptr),
		activeCameraViewportSize(glm::ivec2(1280, 720)),
		prevFrameFence(0),

		bonesUBO(0),
		texturesUBO(0),
		lightmapTextureUBO(0),
		
		activeFramebuffer(-1),
		useFXAA(fxaa)
		
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

		this->clearShader(this->screenShader.get());
		this->clearShader(this->postShader.get());
		this->clearShader(this->compositeShader.get());
	}

	std::unique_ptr<FinalRenderTarget> GPU::createFinalRenderTarget(const std::string& name, unsigned int width, unsigned int height)
	{
		TextureData td;
		TextureData td2;

		std::unique_ptr<FinalRenderTarget> frt = std::make_unique<FinalRenderTarget>();
		frt->texture.name = name;
		frt->texture.frames.push_back(td);
		frt->texture.frames.push_back(td2);
		frt->texture.options = TXT_OPT_HAS_ALPHA | TXT_OPT_CLAMP_UVS | TXT_OPT_CPU_AND_GPU;
		frt->resolution = glm::ivec2(width, height);

		unsigned int fboId = 0;
		glGenFramebuffers(1, &fboId);
		frt->fbo = fboId;

		glGenTextures(1, &frt->texture.frames.at(0).id);
		glGenTextures(1, &frt->texture.frames.at(1).id);

		// color
		glBindTexture(GL_TEXTURE_2D, frt->texture.frames.at(0).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// depth
		glBindTexture(GL_TEXTURE_2D, frt->texture.frames.at(1).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		glBindTexture(GL_TEXTURE_2D, 0); // be safe
		
		this->bindFrameBuffer(frt->fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frt->texture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frt->texture.frames.at(1).id, 0);

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

			SPDLOG_DEBUG("GPU::createFinalRenderTarget: Framebuffer is not complete! Status code: {}", statusString);

			return nullptr;
		}

		// be safe
		this->bindFrameBuffer(0);


		// obtain texture's DSA handle, and set texture's DSA handle as resident so it can be accessed in shaders
		frt->texture.frames.at(0).dsaHandle = glGetTextureHandleARB(frt->texture.frames.at(0).id);
		glMakeTextureHandleResidentARB(frt->texture.frames.at(0).dsaHandle);

		return frt;
	}

	void GPU::freeFinalRenderTarget(FinalRenderTarget* frt)
	{
		glMakeTextureHandleNonResidentARB(frt->texture.frames.at(0).dsaHandle);
		glDeleteTextures(1, &frt->texture.frames.at(0).id);
		glDeleteTextures(1, &frt->texture.frames.at(1).id);
		glDeleteFramebuffers(1, &frt->fbo);
	}

	void GPU::setOpaqueRenderState()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		this->bindFrameBuffer(this->activeRenderTarget->opaqueFBO);		
	}

	void GPU::setAlphaRenderState()
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		glBlendEquation(GL_FUNC_ADD);

		this->bindFrameBuffer(this->activeRenderTarget->alphaFBO);
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
	void GPU::setCompositeRenderState()
	{
		glDepthFunc(GL_ALWAYS);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		this->bindFrameBuffer(this->activeRenderTarget->opaqueFBO);
	}

	void GPU::composeFBOs()
	{
		this->setCompositeRenderState();
		
		this->useShader(this->compositeShader.get());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, this->activeRenderTarget->accumTexture.frames.at(0).id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, this->activeRenderTarget->revealTexture.frames.at(0).id);
		
		this->useMesh(this->screenSpaceMesh.get());

		this->drawGpuMesh();		
	}

	void GPU::setGLDebugMessage(const std::string& message)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			GL_DEBUG_SEVERITY_NOTIFICATION, -1, message.c_str());
	}

	void GPU::drawToFinalRenderTarget(GLuint64 dsaHandle)
	{
		this->useShader(this->screenShader.get());

		this->updateTextureUBO(0, dsaHandle);

		this->useMesh(this->screenSpaceMesh.get());

		this->drawGpuMesh();
	}

	void GPU::drawToScreen(FinalRenderTarget* frt, glm::vec4 tint)
	{
		this->clearScreenBuffer(0.0f, 1.0f, 0.0f, 1.0f);

		this->disableBlend();

		this->useShader(this->postShader.get());

		this->updateTextureUBO(0, frt->texture.frames.at(0).dsaHandle);

		this->setShaderVec4("tint", tint);
		// don't have setShaderVec2 method right now, so use vec3 to get this done
		this->setShaderVec3("resolution", glm::vec3(frt->resolution.x, frt->resolution.y, 0.0f));
		this->setShaderBool("enableFXAA", this->useFXAA);

		this->useMesh(this->screenSpaceMesh.get());

		this->drawGpuMesh();

		this->enableBlend();
	}

	void GPU::setRenderTarget(RenderTarget* rt)
	{
		this->activeRenderTarget = rt;
	}

	void GPU::updateCameraViewportSize(unsigned int width, unsigned int height)
	{
		if (width != this->activeCameraViewportSize.x || height != this->activeCameraViewportSize.y)
		{
			//std::cout << "Altering camera viewport size: " << this->activeCameraViewportSize.x << ", " << this->activeCameraViewportSize.y << std::endl;
			this->activeCameraViewportSize = glm::ivec2(width, height);
			glViewport(0, 0, width, height);
		}
	}

	void GPU::setViewportSize(unsigned int width, unsigned int height)
	{
		glViewport(0, 0, width, height);
	}

	std::unique_ptr<FinalRenderTarget> GPU::updateFinalRenderTargetVPSize(FinalRenderTarget* frt, unsigned int width, unsigned int height)
	{
		std::unique_ptr<FinalRenderTarget> updatedFRT = nullptr;

		if (width != frt->resolution.x || height != frt->resolution.y)
		{
			frt->resolution = glm::ivec2(width, height);

			if (frt->resolution.x != 0 && frt->resolution.y != 0)
			{
				this->freeFinalRenderTarget(frt);
				updatedFRT = this->createFinalRenderTarget(frt->texture.name, width, height);
			}
		}

		glViewport(0, 0, width, height);

		return updatedFRT;
	}

	void GPU::setFinalRenderTarget(FinalRenderTarget* frt)
	{
		glDepthMask(GL_TRUE); // insure we're writing to depth buffer (without this, we had to have two stages 
							// each with a camera for rendering to work right, so i must be disabling it somewhere.
		this->bindFrameBuffer(frt->fbo);
	}

	void GPU::setDefaultFrameBuffer()
	{
		this->bindFrameBuffer(0);
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
		v0.materialUBOIndex = 0;
		this->screenSpaceMeshGeoPool->vertices.push_back(v0);

		// bottom left
		VtxPosNrmlTx v1;
		v1.position = glm::vec3(-1.0f, -1.0f, 0.0f);
		v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v1.textureCoords = glm::vec2(0.0f, 0.0f);
		v1.materialUBOIndex = 0;
		this->screenSpaceMeshGeoPool->vertices.push_back(v1);

		// bottom right
		VtxPosNrmlTx v2;
		v2.position = glm::vec3(1.0f, -1.0f, 0.0f);
		v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v2.textureCoords = glm::vec2(1.0f, 0.0f);
		v2.materialUBOIndex = 0;
		this->screenSpaceMeshGeoPool->vertices.push_back(v2);

		// top right
		VtxPosNrmlTx v3;
		v3.position = glm::vec3(1.0f, 1.0f, 0.0f);
		v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v3.textureCoords = glm::vec2(1.0f, 1.0f);
		v3.materialUBOIndex = 0;
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

	void GPU::resetActives()
	{
		this->activeShader = nullptr;
		this->activeMesh = nullptr;
		this->activeMaterial = nullptr;
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


	void GPU::clearShader(Shader* s)
	{
		glDeleteProgram(s->id);
	}

	void GPU::clearGeoPool(GpuGeoPool ggp)
	{
		glDeleteVertexArrays(1, &ggp.VAO);
		glDeleteBuffers(1, &ggp.VBO);
		glDeleteBuffers(1, &ggp.EBO);
	}

	void GPU::clearTexture(Texture* t)
	{
		for (auto& td : t->frames)
		{
			glMakeTextureHandleNonResidentARB(td.dsaHandle);
			glDeleteTextures(1, &td.id);
		}
	}

	void GPU::clearRenderTarget(RenderTarget* rt)
	{
		glMakeTextureHandleNonResidentARB(rt->opaqueTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleNonResidentARB(rt->depthTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleNonResidentARB(rt->accumTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleNonResidentARB(rt->revealTexture.frames.at(0).dsaHandle);
		glDeleteTextures(1, &rt->opaqueTexture.frames.at(0).id);
		glDeleteTextures(1, &rt->depthTexture.frames.at(0).id);
		glDeleteTextures(1, &rt->accumTexture.frames.at(0).id);
		glDeleteTextures(1, &rt->revealTexture.frames.at(0).id);

		glDeleteFramebuffers(1, &rt->opaqueFBO);
		glDeleteFramebuffers(1, &rt->alphaFBO);
	}

	bool GPU::loadShader(Shader* s)
	{
		int success;
		char infoLog[512];
		std::string infoLogStr = "";

		/////////////////////////////////////////////////////////
		// Vertex Shader
		/////////////////////////////////////////////////////////
		const char* vShaderCode = s->vertCode.c_str();
		unsigned int vertex;
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
		unsigned int geometry = 0;
		if (s->geomCode != "")
		{
			const char* gShaderCode = s->geomCode.c_str();
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
		}


		/////////////////////////////////////////////////////////
		// Fragment Shader
		/////////////////////////////////////////////////////////
		const char* fShaderCode = s->fragCode.c_str();
		unsigned int fragment;
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
		unsigned int id;

		id = glCreateProgram();
		glAttachShader(id, vertex);
		if(geometry > 0)
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

		s->id = id;

		return true;
	}

	RenderTarget GPU::createRenderTarget(const std::string& name, unsigned int width, unsigned int height)
	{
		RenderTarget rt;
		rt.resolution = glm::ivec2(width, height);

		TextureData opaqueTD, depthTD, accumTD, revealTD;
		rt.opaqueTexture.frames.push_back(opaqueTD);
		rt.opaqueTexture.name = name + "_opaqueTexture";
		rt.opaqueTexture.options = TXT_OPT_CPU_AND_GPU | TXT_OPT_CLAMP_UVS;
		
		rt.depthTexture.frames.push_back(depthTD);
		rt.depthTexture.name = name + "_depthTexture";
		rt.depthTexture.options = TXT_OPT_CPU_AND_GPU | TXT_OPT_CLAMP_UVS;
		
		rt.accumTexture.frames.push_back(accumTD);
		rt.accumTexture.name = name + "_accumTexture";
		rt.accumTexture.options = TXT_OPT_HAS_ALPHA | TXT_OPT_CPU_AND_GPU | TXT_OPT_CLAMP_UVS;
		
		rt.revealTexture.frames.push_back(revealTD);
		rt.revealTexture.name = name + "_revealTexture";
		rt.revealTexture.options = TXT_OPT_HAS_ALPHA | TXT_OPT_CPU_AND_GPU | TXT_OPT_CLAMP_UVS;

		glGenFramebuffers(1, &rt.opaqueFBO);
		glGenFramebuffers(1, &rt.alphaFBO);

		glGenTextures(1, &rt.opaqueTexture.frames.at(0).id);
		glGenTextures(1, &rt.depthTexture.frames.at(0).id);
		glGenTextures(1, &rt.accumTexture.frames.at(0).id);
		glGenTextures(1, &rt.revealTexture.frames.at(0).id);
		

		this->updateRenderTarget(&rt);


		// obtain texture's DSA handle
		rt.opaqueTexture.frames.at(0).dsaHandle = glGetTextureHandleARB(rt.opaqueTexture.frames.at(0).id);
		rt.depthTexture.frames.at(0).dsaHandle = glGetTextureHandleARB(rt.depthTexture.frames.at(0).id);
		rt.accumTexture.frames.at(0).dsaHandle = glGetTextureHandleARB(rt.accumTexture.frames.at(0).id);
		rt.revealTexture.frames.at(0).dsaHandle = glGetTextureHandleARB(rt.revealTexture.frames.at(0).id);

		// set texture's DSA handle as resident so it can be accessed in shaders
		glMakeTextureHandleResidentARB(rt.opaqueTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleResidentARB(rt.depthTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleResidentARB(rt.accumTexture.frames.at(0).dsaHandle);
		glMakeTextureHandleResidentARB(rt.revealTexture.frames.at(0).dsaHandle);


		return rt;
	}

	bool GPU::updateRenderTarget(RenderTarget* rt)
	{
		if (rt->resolution.x == 0 || rt->resolution.y == 0)
			return false;

		//
		// Configure opaqueFBO
		//

		// opaque texture
		glBindTexture(GL_TEXTURE_2D, rt->opaqueTexture.frames.at(0).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rt->resolution.x, rt->resolution.y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// depth texture
		glBindTexture(GL_TEXTURE_2D, rt->depthTexture.frames.at(0).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, rt->resolution.x, rt->resolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		// unbind texture (not necessary, but we do it anyway)
		glBindTexture(GL_TEXTURE_2D, 0);

		// associate textures with opaqueFBO
		this->bindFrameBuffer(rt->opaqueFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->opaqueTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depthTexture.frames.at(0).id, 0);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			SPDLOG_DEBUG("GPU::updateRenderTarget: Framebuffer is not complete: 001");
			return false;
		}

		this->bindFrameBuffer(0);


		//
		// Configure alphaFBO
		//

		// accum texture
		glBindTexture(GL_TEXTURE_2D, rt->accumTexture.frames.at(0).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rt->resolution.x, rt->resolution.y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// reveal texture
		glBindTexture(GL_TEXTURE_2D, rt->revealTexture.frames.at(0).id);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rt->resolution.x, rt->resolution.y, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rt->resolution.x, rt->resolution.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// unbind texture (not necessary, but we do it anyway)
		glBindTexture(GL_TEXTURE_2D, 0);

		// associate textures with alphaFBO
		this->bindFrameBuffer(rt->alphaFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->accumTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt->revealTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depthTexture.frames.at(0).id, 0);

		const GLenum transparentDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, transparentDrawBuffers);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			SPDLOG_DEBUG("GPU::updateRenderTarget: Framebuffer is not complete: 002");
			return false;
		}

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
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPos), &gp->vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

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
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrml), &gp->vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

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
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTx), &gp->vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)offsetof(VtxPosNrmlTx, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTx), (void*)offsetof(VtxPosNrmlTx, textureCoords));

		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 1, GL_INT, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, materialUBOIndex));

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
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxLm), &gp->vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, textureCoords));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, lightmapCoords));

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 1, GL_INT, sizeof(VtxPosNrmlTxLm), (void*)offsetof(VtxPosNrmlTxLm, materialUBOIndex));

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
		glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxSkn), &gp->vertices[0], GL_STATIC_DRAW);

		// Generate and bind element buffer object
		glGenBuffers(1, &ggp.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

		// Assign vertex positions to location = 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)0);

		// Assign vertex normals to location = 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, normal));

		// Assign vertex texture coordinates to location = 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, textureCoords));

		// Assign vertex bone ids to location = 3
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, boneIds));

		// Assign vertex weights to location = 4
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, boneWeights));

		// Assign texture id to location = 5
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 1, GL_INT, sizeof(VtxPosNrmlTxSkn), (void*)offsetof(VtxPosNrmlTxSkn, materialUBOIndex));

		// Unbind the vertex array to prevent accidental operations
		glBindVertexArray(0);

		gp->gpuGeoPool = ggp;
	}

	void GPU::loadGeoPool(GeoPool* gp)
	{
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
	}

	void GPU::updateGeoPool(GeoPool* gp)
	{
		GpuGeoPool& ggp = gp->gpuGeoPool.value();

		// Generate and bind vertex attribute array
		glBindVertexArray(ggp.VAO);

		// Bind and update vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, ggp.VBO);

		switch (gp->vtxLayout)
		{
		case VtxLayout::VTX_POS:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPos), &static_cast<GeoPoolT<VtxPos>*>(gp)[0], GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrml), &static_cast<GeoPoolT<VtxPosNrml>*>(gp)[0], GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTx), &static_cast<GeoPoolT<VtxPosNrmlTx>*>(gp)[0], GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX_LM:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxLm), &static_cast<GeoPoolT<VtxPosNrmlTxLm>*>(gp)[0], GL_STATIC_DRAW);
			break;
		case VtxLayout::VTX_POS_NRML_TX_SKN:
			glBufferData(GL_ARRAY_BUFFER, gp->vertexCount() * sizeof(VtxPosNrmlTxSkn), &static_cast<GeoPoolT<VtxPosNrmlTxSkn>*>(gp)[0], GL_STATIC_DRAW);
			break;
		}

		// Bind and update indices buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ggp.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gp->indices.size() * sizeof(unsigned int), &gp->indices[0], GL_STATIC_DRAW);

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

	std::unique_ptr<Texture> GPU::generateEmptyTexture(const std::string& name, unsigned int frameCount, 
		unsigned int width, unsigned int height, int options)
	{
		std::unique_ptr<Texture> t = std::make_unique<Texture>();
		t->name = name;
		t->options = options;

		for (unsigned int i = 0; i < frameCount; i++)
		{
			TextureData td;

			glGenTextures(1, &td.id);
			glBindTexture(GL_TEXTURE_2D, td.id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			if (t->options & TXT_OPT_CLAMP_UVS)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			td.dsaHandle = glGetTextureHandleARB(td.id);
			glMakeTextureHandleResidentARB(td.dsaHandle);

			t->frames.push_back(td);
		}

		return t;
	}

	void GPU::loadTexture(Texture* t)
	{
		for (auto& td : t->frames)
		{
			if (td.primaryImageData.nrComponents == 1)
			{
				td.alphaChannel = false;
				td.primaryImageData.sizedFormat = GL_R8; // 8 bits per channel x1 channel
				td.primaryImageData.format = GL_RED;
			}
			else if (td.primaryImageData.nrComponents == 3)
			{
				td.alphaChannel = false;
				td.primaryImageData.sizedFormat = GL_RGB8; // 8 bits per channel x3 channels
				td.primaryImageData.format = GL_RGB;
			}
			else if (td.primaryImageData.nrComponents == 4)
			{
				td.alphaChannel = true;
				td.primaryImageData.sizedFormat = GL_RGBA8; // 8 bits per channel x4 channels
				td.primaryImageData.format = GL_RGBA;
			}


			// create a texture buffer and bind it to context
			glGenTextures(1, &td.id);
			glBindTexture(GL_TEXTURE_2D, td.id);

			// load data into the buffer
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				td.primaryImageData.sizedFormat,
				td.primaryImageData.width,
				td.primaryImageData.height,
				0,
				td.primaryImageData.format,
				GL_UNSIGNED_BYTE,
				td.primaryImageData.data
			);

			//// auto generate mipmap levels for texture
			glGenerateMipmap(GL_TEXTURE_2D);

			// set texture parameters
			if (t->options & TXT_OPT_CLAMP_UVS)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			
			if (t->options & TXT_OPT_DISABLE_FILTER)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}

			// obtain texture's DSA handle
			td.dsaHandle = glGetTextureHandleARB(td.id);

			// set texture's DSA handle as resident so it can be accessed in shaders
			glMakeTextureHandleResidentARB(td.dsaHandle);

			if(!(t->options & TXT_OPT_CPU_AND_GPU))
				stbi_image_free(td.primaryImageData.data);
		}	
	}

	void GPU::loadFontBitmapTexture(FontBitmap* fb)
	{
		Texture t;
		t.name = fb->fontName + "_fontTexture";

		TextureData td;
		glGenTextures(1, &td.id);
		glBindTexture(GL_TEXTURE_2D, td.id);
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
		// TODO: verify these don't cause trouble
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// reset pack alignment to default
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		// obtain texture's DSA handle
		td.dsaHandle = glGetTextureHandleARB(td.id);

		// set texture's DSA handle as resident so it can be accessed in shaders
		glMakeTextureHandleResidentARB(td.dsaHandle);

		// push this texture data object as a frame into texture
		t.frames.push_back(td);

		fb->texture = t;
	}

	const Shader* const GPU::getActiveShader() const
	{
		return this->activeShader;
	}

	const Mesh* const GPU::getActiveMesh() const
	{
		return this->activeMesh;
	}

	const Material* const GPU::getActiveMaterial() const
	{
		return this->activeMaterial;
	}

	glm::ivec2 GPU::getActiveCameraViewportSize()
	{
		return this->activeCameraViewportSize;
	}

	void GPU::useShader(Shader* s)
	{
		if (s == nullptr || this->activeShader == s)
			return;

		this->activeShader = s;
		glUseProgram(s->id);
	}

	void GPU::setShaderBool(const std::string& name, bool value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform1i(this->activeShader->uniformLocations[name], (int)value);
	}

	void GPU::setShaderInt(const std::string& name, int value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform1i(this->activeShader->uniformLocations[name], value);
	}

	void GPU::setShaderUInt(const std::string &name, uint64_t value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform1ui(this->activeShader->uniformLocations[name], value);
	}

	void GPU::setShaderFloat(const std::string& name, float value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform1f(this->activeShader->uniformLocations[name], value);
	}

	void GPU::setShaderFloatArray(const std::string &name, const std::vector<float>& value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform1fv(this->activeShader->uniformLocations[name], value.size(), &value[0]);
	}

	void GPU::setShaderMat4(const std::string& name, const glm::mat4& value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniformMatrix4fv(this->activeShader->uniformLocations[name], 1, GL_FALSE, glm::value_ptr(value));
	}

	void GPU::setShaderVec3(const std::string &name, const glm::vec3& value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform3fv(this->activeShader->uniformLocations[name], 1, &value[0]);
	}

	void GPU::setShaderVec3Array(const std::string &name, const std::vector<glm::vec3>& value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform3fv(this->activeShader->uniformLocations[name], value.size(), glm::value_ptr(value[0]));
	}

	void GPU::setShaderVec4(const std::string &name, const glm::vec4& value)
	{
		if (!this->activeShader->uniformLocations.count(name) == 1)
			this->activeShader->uniformLocations[name] = glGetUniformLocation(this->activeShader->id, name.c_str());

		glUniform4fv(this->activeShader->uniformLocations[name], 1, &value[0]);
	}


	void GPU::useMesh(Mesh* m)
	{
		if (!m || m == this->activeMesh)
			return;

		this->activeMesh = m;
		glBindVertexArray(m->getGpuMesh()->VAO);
	}

	void GPU::setActiveMaterial(Material* m)
	{
		this->activeMaterial = m;
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

	void GPU::clearRenderTargetBuffers(float r, float g, float b, float a)
	{
		this->bindFrameBuffer(this->activeRenderTarget->opaqueFBO);
		this->clearBuffers(r,g,b,a);

		this->bindFrameBuffer(this->activeRenderTarget->alphaFBO);
		glClearBufferfv(GL_COLOR, 0, &this->zeroFillerVec[0]);
		glClearBufferfv(GL_COLOR, 1, &this->oneFillerVec[0]);
	}

	void GPU::clearFinalRenderTarget(FinalRenderTarget* frt, glm::vec4 color)
	{
		if (!frt)
			return;

		this->bindFrameBuffer(frt->fbo);
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

	void GPU::drawGpuMesh()
	{
		glDrawElements(GL_TRIANGLES, this->activeMesh->getGpuMesh()->indiceCount, GL_UNSIGNED_INT, 0);
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
		this->screenShader->name = "screen";

		this->screenShader->vertCode = R"GLSL(
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

		this->screenShader->fragCode = R"GLSL(
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

		this->loadShader(this->screenShader.get());


		////////////////////////////////////////////////////////
		// Post Shader
		////////////////////////////////////////////////////////
		this->postShader->name = "post";

		this->postShader->vertCode = R"GLSL(
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

		this->postShader->fragCode = R"GLSL(
#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

in vec2 TexCoords;
in vec2 LMTexCoords;
flat in uint TexId;

uniform vec4 tint;
uniform vec3 resolution;
uniform bool enableFXAA;

const int MAX_TEXTURE_SLOTS = 250;
layout (std140, binding = 0) uniform TexturesUBO
{
    sampler2D tex[MAX_TEXTURE_SLOTS];
};

out vec4 FragColor;

vec3 applyFXAA(sampler2D tex, vec2 uv, vec2 res)
{
    vec2 inverseResolution = 1.0 / res;

    // Sample positions
    vec3 rgbNW = texture(tex, uv + (vec2(-1.0, -1.0) * inverseResolution)).rgb;
    vec3 rgbNE = texture(tex, uv + (vec2( 1.0, -1.0) * inverseResolution)).rgb;
    vec3 rgbSW = texture(tex, uv + (vec2(-1.0,  1.0) * inverseResolution)).rgb;
    vec3 rgbSE = texture(tex, uv + (vec2( 1.0,  1.0) * inverseResolution)).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    // Luma (brightness)
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    float lumaM  = dot(rgbM,  vec3(0.299, 0.587, 0.114));

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Early exit: if contrast is too low, skip FXAA
    if (lumaMax - lumaMin < max(0.0312, lumaMax * 0.125))
        return rgbM;
    
    // Edge detection
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * 0.5),
        1.0 / 32.0
    );
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * inverseResolution;

    // Sample along the edge direction
    vec3 rgbA = 0.5 * (
        texture(tex, uv + dir * (1.0/3.0 - 0.5)).rgb +
        texture(tex, uv + dir * (2.0/3.0 - 0.5)).rgb
    );
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, uv + dir * -0.5).rgb +
        texture(tex, uv + dir * 0.5).rgb
    );

    // Choose based on brightness range
    float lumaB = dot(rgbB, vec3(0.299, 0.587, 0.114));
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        return rgbA;
    else
        return rgbB;
}

void main()
{	
	if (enableFXAA)
	{
		vec3 fxaaResult = applyFXAA(tex[TexId], TexCoords, vec2(resolution.x, resolution.y));
		FragColor = mix(vec4(fxaaResult, 1.0), vec4(tint.rgb, 1.0), tint.a);
	}
	else
	{
		vec4 texColor = texture(tex[TexId], TexCoords);
		FragColor = mix(texColor, vec4(tint.rgb, 1.0), tint.a);
	}
}
)GLSL";

		this->loadShader(this->postShader.get());


		////////////////////////////////////////////////////////
		// Composite Shader
		////////////////////////////////////////////////////////
		this->compositeShader->name = "composite";

		this->compositeShader->vertCode = R"GLSL(
#version 460 core

layout (location = 0) in vec3 position;

void main()
{
	gl_Position = vec4(position, 1.0f);
}
)GLSL";

		this->compositeShader->fragCode = R"GLSL(
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

		this->loadShader(this->compositeShader.get());

	}



} // END NAMESPACE