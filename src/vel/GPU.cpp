#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <glm/gtx/string_cast.hpp>
#include "stb_headers/stb_image.h"

#include "vel/GPU.h"
#include "vel/Vertex.h"
#include "vel/functions.h"
#include "vel/Log.h"

namespace vel
{
	GPU::GPU(bool fxaa) :
		screenShader(nullptr),
		postShader(nullptr),
		compositeShader(nullptr),
		activeShader(nullptr),
		activeMesh(nullptr),
		activeMaterial(nullptr),
		activeRenderTarget(nullptr),
		screenSpaceMesh(Mesh("screenSpaceMesh")),
		zeroFillerVec(glm::vec4(0.0f)),
		oneFillerVec(1.0f),
		activeClearColorValues(glm::vec4(0.0f)),
		activeViewportSize(glm::ivec2(1280,720)),
		activeFramebuffer(-1),

		renderedFBO(nullptr),
		renderedFBOTexture(nullptr),
		viewportSizeAltered(false),

		useFXAA(fxaa)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // why?

        //this->enableDepthTest();
		this->enableBackfaceCulling();
		this->initBoneUBO();
		this->initTextureUBO();
		this->initLightMapTextureUBO();
		this->initScreenSpaceMesh();

		this->createRenderedFBO(this->activeViewportSize.x, this->activeViewportSize.y);
	}

	GPU::~GPU()
	{
		this->clearMesh(&this->screenSpaceMesh);
	}

	void GPU::createRenderedFBO(unsigned int width, unsigned int height)
	{
		TextureData td;
		TextureData td2;

		this->renderedFBOTexture = std::make_unique<Texture>();
		this->renderedFBOTexture->name = "renderedFBOTexture";
		this->renderedFBOTexture->frames.push_back(td);
		this->renderedFBOTexture->frames.push_back(td2);
		this->renderedFBOTexture->alphaChannel = true;
		this->renderedFBOTexture->freeAfterGPULoad = false;
		this->renderedFBOTexture->uvWrapping = 0;

		unsigned int fboId = 0;
		glGenFramebuffers(1, &fboId);
		this->renderedFBO = std::make_unique<unsigned int>(fboId);

		glGenTextures(1, &this->renderedFBOTexture->frames.at(0).id);
		glGenTextures(1, &this->renderedFBOTexture->frames.at(1).id);

		// color
		glBindTexture(GL_TEXTURE_2D, this->renderedFBOTexture->frames.at(0).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// depth
		glBindTexture(GL_TEXTURE_2D, this->renderedFBOTexture->frames.at(1).id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		glBindTexture(GL_TEXTURE_2D, 0); // be safe

		glBindFramebuffer(GL_FRAMEBUFFER, *this->renderedFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->renderedFBOTexture->frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->renderedFBOTexture->frames.at(1).id, 0);

		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);


		// verify success
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status code: " << status << std::endl;

			switch (status)
			{
			case GL_FRAMEBUFFER_UNDEFINED:
				std::cout << "GL_FRAMEBUFFER_UNDEFINED" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				std::cout << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				std::cout << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
				break;
			default:
				std::cout << "Unknown error" << std::endl;
			}

			std::cin.get();
			exit(EXIT_FAILURE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // be safe


		// obtain texture's DSA handle, and set texture's DSA handle as resident so it can be accessed in shaders
		this->renderedFBOTexture->frames.at(0).dsaHandle = glGetTextureHandleARB(this->renderedFBOTexture->frames.at(0).id);
		glMakeTextureHandleResidentARB(this->renderedFBOTexture->frames.at(0).dsaHandle);
	}

	void GPU::setOpaqueRenderState()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		if (this->activeFramebuffer != this->activeRenderTarget->opaqueFBO)
		{
			this->activeFramebuffer = this->activeRenderTarget->opaqueFBO;
			glBindFramebuffer(GL_FRAMEBUFFER, this->activeRenderTarget->opaqueFBO);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		
	}

	void GPU::setAlphaRenderState()
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		glBlendEquation(GL_FUNC_ADD);

		if (this->activeFramebuffer != this->activeRenderTarget->alphaFBO)
		{
			this->activeFramebuffer = this->activeRenderTarget->alphaFBO;
			glBindFramebuffer(GL_FRAMEBUFFER, this->activeRenderTarget->alphaFBO);
			//glClearBufferfv(GL_COLOR, 0, &this->zeroFillerVec[0]);
			//glClearBufferfv(GL_COLOR, 1, &this->oneFillerVec[0]);
		}
		
	}

	void GPU::setCompositeRenderState()
	{
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (this->activeFramebuffer != this->activeRenderTarget->opaqueFBO)
		{
			this->activeFramebuffer = this->activeRenderTarget->opaqueFBO;
			glBindFramebuffer(GL_FRAMEBUFFER, this->activeRenderTarget->opaqueFBO);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	void GPU::composeFBOs()
	{
		this->setCompositeRenderState();
		
		this->useShader(this->compositeShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, this->activeRenderTarget->accumTexture.frames.at(0).id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, this->activeRenderTarget->revealTexture.frames.at(0).id);
		
		this->useMesh(&this->screenSpaceMesh);

		this->drawGpuMesh();		
	}

	void GPU::setGLDebugMessage(const std::string& message)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			GL_DEBUG_SEVERITY_NOTIFICATION, -1, message.c_str());
	}

	void GPU::setDefaultWhiteTextureHandle(uint64_t th)
	{
		this->defaultWhiteTextureHandle = th;
	}

	void GPU::setScreenShader(Shader* s)
	{
		this->screenShader = s;
	}

	void GPU::setPostShader(Shader* s)
	{
		this->postShader = s;
	}

	void GPU::setCompositeShader(Shader* s)
	{
		this->compositeShader = s;
	}

	void GPU::drawToRenderedFBO(GLuint64 dsaHandle)
	{
		this->useShader(this->screenShader);

		this->updateTextureUBO(0, dsaHandle);

		this->updateLightmapTextureUBO(this->defaultWhiteTextureHandle);

		this->useMesh(&this->screenSpaceMesh);

		this->drawGpuMesh();
	}

	void GPU::drawToScreen(glm::vec4 tint)
	{
		this->useShader(this->postShader);

		this->updateTextureUBO(0, this->renderedFBOTexture->frames.at(0).dsaHandle);

		this->setShaderVec4("tint", tint);
		// don't have setShaderVec2 method right now, so use vec3 to get this done
		this->setShaderVec3("resolution", glm::vec3(this->activeViewportSize.x, this->activeViewportSize.y, 0.0f));
		this->setShaderBool("enableFXAA", this->useFXAA);

		this->updateLightmapTextureUBO(this->defaultWhiteTextureHandle);

		this->useMesh(&this->screenSpaceMesh);

		this->drawGpuMesh();
	}

	void GPU::setRenderTarget(RenderTarget* rt)
	{
		this->activeRenderTarget = rt;
	}

	void GPU::updateViewportSize(unsigned int width, unsigned int height)
	{
		if (width != this->activeViewportSize.x || height != this->activeViewportSize.y)
		{
			this->activeViewportSize = glm::ivec2(width, height);
			this->updateRenderedViewportSize();
			glViewport(0, 0, width, height);

		}
	}

	void GPU::updateRenderedViewportSize()
	{
		if (this->activeViewportSize.x == 0 || this->activeViewportSize.y == 0)
			return;

		this->clearRenderedFBO();
		this->createRenderedFBO(this->activeViewportSize.x, this->activeViewportSize.y);
	}

	void GPU::clearRenderedFBO()
	{
		glMakeTextureHandleNonResidentARB(this->renderedFBOTexture->frames.at(0).dsaHandle);
		glDeleteTextures(1, &this->renderedFBOTexture->frames.at(0).id);
		glDeleteTextures(1, &this->renderedFBOTexture->frames.at(1).id);

		glDeleteFramebuffers(1, this->renderedFBO.get());
	}

	void GPU::setRenderedFBO()
	{
		if (this->activeFramebuffer != *this->renderedFBO)
			glBindFramebuffer(GL_FRAMEBUFFER, *this->renderedFBO);
	}

	void GPU::setDefaultFrameBuffer()
	{
		if (this->activeFramebuffer != 0)
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GPU::initScreenSpaceMesh()
	{
		// top left
		Vertex v0;
		v0.position = glm::vec3(-1.0f, 1.0f, 0.0f);
		v0.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v0.textureCoordinates = glm::vec2(0.0f, 1.0f);
		v0.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v0.materialUBOIndex = 0;

		// bottom left
		Vertex v1;
		v1.position = glm::vec3(-1.0f, -1.0f, 0.0f);
		v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v1.textureCoordinates = glm::vec2(0.0f, 0.0f);
		v1.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v1.materialUBOIndex = 0;

		// bottom right
		Vertex v2;
		v2.position = glm::vec3(1.0f, -1.0f, 0.0f);
		v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v2.textureCoordinates = glm::vec2(1.0f, 0.0f);
		v2.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v2.materialUBOIndex = 0;

		// top right
		Vertex v3;
		v3.position = glm::vec3(1.0f, 1.0f, 0.0f);
		v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v3.textureCoordinates = glm::vec2(1.0f, 1.0f);
		v3.lightmapCoordinates = glm::vec2(0.0f, 0.0f);
		v3.materialUBOIndex = 0;

		std::vector<Vertex> vs = { v0, v1, v2, v3 };
		this->screenSpaceMesh.setVertices(vs);

		std::vector<unsigned int> is = { 0,1,2,0,2,3 };
		this->screenSpaceMesh.setIndices(is);

		this->loadMesh(&this->screenSpaceMesh);
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
		glBufferData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * 2, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, this->lightmapTextureUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::updateLightmapTextureUBO(GLuint64 dsaHandle)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->lightmapTextureUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GLuint64) * 2, (void*)&dsaHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::initTextureUBO()
	{
		const int MAX_SUPPORTED_TEXTURES = 250;
		glGenBuffers(1, &this->texturesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, this->texturesUBO);
		glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_TEXTURES * sizeof(GLuint64) * 2, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->texturesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void GPU::updateTextureUBO(unsigned int index, GLuint64 dsaHandle)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->texturesUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GLuint64) * index * 2, sizeof(GLuint64) * 2, (void*)&dsaHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}



	void GPU::initBoneUBO()
	{
		const int MAX_SUPPORTED_BONES = 200;
		glGenBuffers(1, &this->bonesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, this->bonesUBO);
		glBufferData(GL_UNIFORM_BUFFER, MAX_SUPPORTED_BONES * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, this->bonesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		//std::cout << "bonesUBO: " << this->bonesUBO << std::endl;
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

	void GPU::clearMesh(Mesh* m)
	{
		if (m->getGpuMesh())
		{
			glDeleteVertexArrays(1, &m->getGpuMesh().value().VAO);
			glDeleteBuffers(1, &m->getGpuMesh().value().VBO);
			glDeleteBuffers(1, &m->getGpuMesh().value().EBO);
		}
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

	void GPU::loadShader(Shader* s)
	{
		int success;
		char infoLog[512];


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
		if (!success) {
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << "\n";
			std::cin.get();
			exit(EXIT_FAILURE);
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
			if (!success) {
				glGetShaderInfoLog(geometry, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << "\n";
				std::cin.get();
				exit(EXIT_FAILURE);
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
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << "\n";
			std::cin.get();
			exit(EXIT_FAILURE);
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
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(geometry);
		glDeleteShader(fragment);

		s->id = id;
	}

	RenderTarget GPU::createRenderTarget(const std::string& name, unsigned int width, unsigned int height)
	{
		RenderTarget rt;
		rt.resolution = glm::ivec2(width, height);

		TextureData opaqueTD, depthTD, accumTD, revealTD;
		rt.opaqueTexture.frames.push_back(opaqueTD);
		rt.opaqueTexture.alphaChannel = false;
		rt.opaqueTexture.freeAfterGPULoad = false;
		rt.opaqueTexture.uvWrapping = 0;
		rt.opaqueTexture.name = name + "_opaqueTexture";

		rt.depthTexture.frames.push_back(depthTD);
		rt.depthTexture.alphaChannel = false;
		rt.depthTexture.freeAfterGPULoad = false;
		rt.depthTexture.uvWrapping = 0;
		rt.depthTexture.name = name + "_depthTexture";

		rt.accumTexture.frames.push_back(accumTD);
		rt.accumTexture.alphaChannel = true;
		rt.accumTexture.freeAfterGPULoad = false;
		rt.accumTexture.uvWrapping = 0;
		rt.accumTexture.name = name + "_accumTexture";

		rt.revealTexture.frames.push_back(revealTD);
		rt.revealTexture.alphaChannel = true;
		rt.revealTexture.freeAfterGPULoad = false;
		rt.revealTexture.uvWrapping = 0;
		rt.revealTexture.name = name + "_revealTexture";

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

	void GPU::updateRenderTarget(RenderTarget* rt)
	{
		if (rt->resolution.x == 0 || rt->resolution.y == 0)
			return;

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
		glBindFramebuffer(GL_FRAMEBUFFER, rt->opaqueFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->opaqueTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depthTexture.frames.at(0).id, 0);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! 234sdgf" << std::endl;
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rt->resolution.x, rt->resolution.y, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// unbind texture (not necessary, but we do it anyway)
		glBindTexture(GL_TEXTURE_2D, 0);

		// associate textures with alphaFBO
		glBindFramebuffer(GL_FRAMEBUFFER, rt->alphaFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->accumTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt->revealTexture.frames.at(0).id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depthTexture.frames.at(0).id, 0);

		const GLenum transparentDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, transparentDrawBuffers);

		// verify success
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! gh66uhm" << std::endl;
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);





	}

	void GPU::loadMesh(Mesh* m)
	{
		GpuMesh gm = GpuMesh();
		gm.indiceCount = (GLsizei)m->getIndices().size();

		// Generate and bind vertex attribute array
		glGenVertexArrays(1, &gm.VAO);
		glBindVertexArray(gm.VAO);

		// Generate and bind vertex buffer object
		glGenBuffers(1, &gm.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, gm.VBO);
		glBufferData(GL_ARRAY_BUFFER, m->getVertices().size() * sizeof(Vertex), &m->getVertices()[0], GL_STATIC_DRAW);

		// Generate and bind element buffer object
		glGenBuffers(1, &gm.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->getIndices().size() * sizeof(unsigned int), &m->getIndices()[0], GL_STATIC_DRAW);

		// Assign vertex positions to location = 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		// Assign vertex normals to location = 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		// Assign vertex texture coordinates to location = 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textureCoordinates));

		// Assign vertex lightmap texture coordinates to location = 3
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, lightmapCoordinates));

		// Assign texture id to location = 4
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, materialUBOIndex));

		// Assign vertex bone ids to location = 5 (and 6 for second array element)
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, weights.ids));
		glEnableVertexAttribArray(6);
		glVertexAttribIPointer(6, 4, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, weights.ids) + 16));

		// Assign vertex weights to location = 7 (and 8 for second array element)
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights.weights));
		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, weights.weights) + 16));

		// Unbind the vertex array to prevent accidental operations
		glBindVertexArray(0);

		m->setGpuMesh(gm);
	}

	void GPU::updateMesh(Mesh* m)
	{
		auto& gm = m->getGpuMesh().value();
		gm.indiceCount = (GLsizei)m->getIndices().size();

		// Generate and bind vertex attribute array
		glBindVertexArray(gm.VAO);

		// Bind and update vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, gm.VBO);
		glBufferData(GL_ARRAY_BUFFER, m->getVertices().size() * sizeof(Vertex), &m->getVertices()[0], GL_STATIC_DRAW);

		// Bind and update indices buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->getIndices().size() * sizeof(unsigned int), &m->getIndices()[0], GL_STATIC_DRAW);

		// Unbind the vertex array to prevent accidental operations
		glBindVertexArray(0);
	}

	void GPU::loadTexture(Texture* t)
	{
		for (auto& td : t->frames)
		{
			//// create a texture buffer and bind it to context
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
			if (t->uvWrapping == 0)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			
			

			// obtain texture's DSA handle
			td.dsaHandle = glGetTextureHandleARB(td.id);

			// set texture's DSA handle as resident so it can be accessed in shaders
			glMakeTextureHandleResidentARB(td.dsaHandle);

			if(t->freeAfterGPULoad)
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

	glm::ivec2 GPU::getActiveViewportSize()
	{
		return this->activeViewportSize;
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
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// https://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha/#toc5
	// with this method we can enable transparent framebuffers in glfw, clear the framebuffers
	// that we render as textures with alpha component, render those, then use this blending 
	// method when we draw them to the screenbuffer
	void GPU::enableBlend2()
	{
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
		glBindFramebuffer(GL_FRAMEBUFFER, this->activeRenderTarget->opaqueFBO);
		this->clearBuffers(r,g,b,a);

		glBindFramebuffer(GL_FRAMEBUFFER, this->activeRenderTarget->alphaFBO);
		glClearBufferfv(GL_COLOR, 0, &this->zeroFillerVec[0]);
		glClearBufferfv(GL_COLOR, 1, &this->oneFillerVec[0]);
	}

	void GPU::clearRenderedFBO(float r, float g, float b, float a)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, *this->renderedFBO);
		this->clearBuffers(r, g, b, a);
	}

	void GPU::clearScreenBuffer(float r, float g, float b, float a)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		this->clearBuffers(r, g, b, a);
	}

	void GPU::drawLinesOnly()
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	void GPU::finish()
	{
		glFinish();
	}

	void GPU::drawGpuMesh()
	{
		// The naive approach. But after researching how to optimize this for 3 days I decided to leave it alone
		// until there's an actual reason to complicate things.
		//
		// TODO: Coming back to this years later, we should definitely refactor so that we don't have to swap
		// buffer objects for each different object, just pack all of our vertex data into the same buffer
		// and draw elements of that object individually using glDrawElementsBaseVertex()... I think that's the right
		// method, need to confirm... doing this will greatly reduce the number of state switches, especially since
		// we now use DSA textures
		//
		// OK, Years later again, I looked into this after implementing a material system, and I have actor objects
		// setup in 2d unordered_maps where the final value is a vector and I only switch shader program and vao
		// when necessary, but each unique mesh is still it's own buffer object. My intention was to implement what
		// I spoke of above, but honestly...if I can accomplish what I want to accomplish with this paradigm, then
		// it really doesn't necessitate the extra work, because I would have to rework quite a bit of logic, and
		// it's just not worth the time if it's not required
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

}