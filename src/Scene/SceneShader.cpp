#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	unsigned int Scene::getShaderProgramId(uint32_t flags)
	{
		auto it = this->shaderHandleMap.find(flags);
		if (it != this->shaderHandleMap.end())
		{
			SPDLOG_DEBUG("Scene::getShaderProgramId(): Existing Shader, bypass reload: {}", flags);
			return this->shaders[it->second].programId;
		}
		else
		{
			SPDLOG_DEBUG("Scene::getShaderProgramId(): Loading new Shader: {}", flags);
			return this->shaders[this->generateShader(flags)].programId;
		}
	}

	shader_handle Scene::generateLineShader(uint32_t flags)
	{
		// TODO: things

		return 0;
	}

	shader_handle Scene::generateShader(uint32_t flags)
	{
		if (flags & MTLFLG_IS_LINE)
			return this->generateLineShader(flags);

		std::string vertCode = R"GLSL(#version 460 core
#extension GL_ARB_gpu_shader_int64 : require

struct DrawBucketCommand
{
	uint	count;
	uint	instanceCount;
	uint	firstIndex;
	int		baseVertex;
	uint	baseInstance;
	uint	materialIndex;
	uint	activeFrame;
};

struct ActorGpuData
{
	mat4		model;
	vec4		colorMultiplier;
	uint64_t	lightmapHandle;
	uint		ambientCubeOffset;
	uint		boneMatrixOffset;
};

layout(std430, binding = 0) readonly buffer DrawCommandBuffer
{
	DrawBucketCommand drawCommands[];
};

layout(std140, binding = 1) uniform CameraData
{
	mat4 view;
	mat4 projection;
};

layout(std430, binding = 2) readonly buffer ActorDataBuffer
{
	ActorGpuData actors[];
};

layout(std430, binding = 6) readonly buffer ActorBoneMatrixBuffer
{
	mat4 actorBoneMatrices[];
};

layout(location = 0) in vec3	aPos;
layout(location = 1) in vec3	aNormal;
layout(location = 2) in vec2	aTexCoord;
layout(location = 3) in vec2	aLightMapCoord;
layout(location = 4) in uvec4	aBoneIds;
layout(location = 5) in vec4	aBoneWeights;

out vec2		fragTexCoord;
out vec2		fragTexCoordLightMap;
out vec3		fragWorldPos;
out vec3		fragNormal;

flat out uint	fragActorIndex;
flat out uint	fragMaterialIndex;
flat out uint	fragActiveFrame;

void main() {

	const uint actorIndex		= uint(gl_BaseInstance);
	const uint drawIndex		= uint(gl_DrawID);
    const uint materialIndex	= drawCommands[drawIndex].materialIndex;
    const uint activeFrame		= drawCommands[drawIndex].activeFrame;
    const ActorGpuData actor	= actors[actorIndex];

    fragActorIndex				= actorIndex;
    fragMaterialIndex			= materialIndex;
    fragActiveFrame				= activeFrame;)GLSL";

		if (flags & MTLFLG_HAS_TEXTURES)
		{
			vertCode += R"GLSL(
	fragTexCoord				= aTexCoord;)GLSL";
		}

		if (flags & MTLFLG_HAS_LIGHTMAP)
		{
			vertCode += R"GLSL(
	fragTexCoordLightMap		= aLightMapCoord;)GLSL";
		}
	
		vertCode += R"GLSL(

	vec4 localPosition			= vec4(aPos, 1.0);
    vec3 localNormal			= aNormal;)GLSL";

		if (flags & MTLFLG_IS_SKINNED)
		{
			vertCode += R"GLSL(

	const mat4 skinMatrix		= actorBoneMatrices[actor.boneMatrixOffset + inBoneIds.x] * aBoneWeights.x
								+ actorBoneMatrices[actor.boneMatrixOffset + inBoneIds.y] * aBoneWeights.y
								+ actorBoneMatrices[actor.boneMatrixOffset + inBoneIds.z] * aBoneWeights.z
								+ actorBoneMatrices[actor.boneMatrixOffset + inBoneIds.w] * aBoneWeights.w;

	localPosition				= skinMatrix * localPosition;
    localNormal					= mat3(skinMatrix) * localNormal;)GLSL";
		}

		vertCode += R"GLSL(

	const vec4 worldPosition	= actor.model * localPosition

    fragWorldPos				= worldPosition.xyz;
    fragNormal					= normalize(mat3(actor.model) * localNormal); // uniform scale only

    gl_Position					= projection * view * worldPosition;
})GLSL";

	}






} // END NAMESPACE