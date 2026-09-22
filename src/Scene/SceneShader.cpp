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
		//
		// TODO: Intentionally not impementing this until we get the regular pipeline working
		//

		return 0;
	}

	void Scene::generateVertexShader(uint32_t flags, std::string& code)
	{
		code =	"#version 460 core\n"
				"#extension GL_ARB_gpu_shader_int64 : require\n\n";

		if (flags & MTLFLG_HAS_TEXTURES)
			code += "#define HAS_TEXTURES\n";
		if (flags & MTLFLG_HAS_LIGHTMAP)
			code += "#define HAS_LIGHTMAP\n";
		if (flags & MTLFLG_IS_SKINNED)
			code += "#define IS_SKINNED\n";

		code += R"GLSL(
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

const uint ACTFLG_BILLBOARD			= 1u << 6;
const uint ACTFLG_BILLBOARD_LOCK_Y	= 1u << 7;

struct ActorGpuData
{
	mat4		model;
	vec4		colorMultiplier;
	uint64_t	lightmapHandle;
	uint flags;
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

#ifdef HAS_TEXTURES
out vec2		fragTexCoord;
#endif
#ifdef HAS_LIGHTMAP
out vec2		fragTexCoordLightMap;
#endif
out vec3		fragWorldPos;
out vec3		fragNormal;

flat out uint	fragActorIndex;
flat out uint	fragMaterialIndex;
flat out uint	fragActiveFrame;

void main() 
{
	const uint actorIndex		= uint(gl_BaseInstance);
	const uint drawIndex		= uint(gl_DrawID);
    const uint materialIndex	= drawCommands[drawIndex].materialIndex;
    const uint activeFrame		= drawCommands[drawIndex].activeFrame;
    const ActorGpuData actor	= actors[actorIndex];

    fragActorIndex				= actorIndex;
    fragMaterialIndex			= materialIndex;
    fragActiveFrame				= activeFrame;
	#ifdef HAS_TEXTURES
	fragTexCoord				= aTexCoord;
	#endif
	#ifdef HAS_LIGHTMAP
	fragTexCoordLightMap		= aLightMapCoord;
	#endif

	vec4 localPosition			= vec4(aPos, 1.0);
    vec3 localNormal			= aNormal;

	#ifdef IS_SKINNED
	const mat4 skinMatrix		= actorBoneMatrices[actor.boneMatrixOffset + aBoneIds.x] * aBoneWeights.x
								+ actorBoneMatrices[actor.boneMatrixOffset + aBoneIds.y] * aBoneWeights.y
								+ actorBoneMatrices[actor.boneMatrixOffset + aBoneIds.z] * aBoneWeights.z
								+ actorBoneMatrices[actor.boneMatrixOffset + aBoneIds.w] * aBoneWeights.w;
	localPosition				= skinMatrix * localPosition;
    localNormal					= mat3(skinMatrix) * localNormal;
	#endif


	vec4 worldPosition;
	vec3 worldNormal;

	if ((actor.flags & ACTFLG_BILLBOARD) != 0u)
	{
		const vec3 actorPosition = actor.model[3].xyz;
		const vec2 billboardScale = vec2(length(actor.model[0].xyz), length(actor.model[1].xyz));

		const vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
		const vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
		const vec3 cameraBackward = vec3(view[0][2], view[1][2], view[2][2]);

		vec3 billboardRight = cameraRight;
		vec3 billboardUp = cameraUp;
		vec3 billboardBackward = cameraBackward;

		if ((actor.flags & ACTFLG_BILLBOARD_LOCK_Y) != 0u)
		{
			billboardUp = vec3(0.0, 1.0, 0.0);

			vec3 horizontalBackward = vec3(cameraBackward.x, 0.0, cameraBackward.z);

			if (dot(horizontalBackward, horizontalBackward) < 0.000001)
			{
				horizontalBackward = vec3(actor.model[2].x, 0.0, actor.model[2].z);

				if (dot(horizontalBackward, horizontalBackward) < 0.000001)
					horizontalBackward = vec3(0.0, 0.0, 1.0);
			}

			billboardBackward = normalize(horizontalBackward);
			billboardRight = cross(billboardUp, billboardBackward);
		}

		const mat3 billboardRotation = mat3(billboardRight, billboardUp, billboardBackward);

		const vec3 billboardLocalPosition = vec3(
			localPosition.x * billboardScale.x,
			localPosition.y * billboardScale.y,
			localPosition.z
		);

		worldPosition = vec4(actorPosition + billboardRotation * billboardLocalPosition, 1.0);
		worldNormal = billboardRotation * localNormal;
	}
	else
	{
		worldPosition = actor.model * localPosition;
		worldNormal = normalize(mat3(actor.model) * localNormal);
	}

	fragWorldPos = worldPosition.xyz;
	fragNormal = worldNormal;

	gl_Position = projection * view * worldPosition;
	




})GLSL";
	}

	void Scene::generateFragmentShader(uint32_t flags, std::string& code)
	{
		code = "#version 460 core\n"
			"#extension GL_ARB_bindless_texture : require\n"
			"#extension GL_ARB_gpu_shader_int64 : require\n\n";

		if (flags & MTLFLG_HAS_TEXTURES)
			code += "#define HAS_TEXTURES\n";
		if (flags & MTLFLG_HAS_LIGHTMAP)
			code += "#define HAS_LIGHTMAP\n";
		if (flags & MTLFLG_HAS_AMBIENT_CUBE)
			code += "#define HAS_AMBIENT_CUBE\n";
		if (flags & MTLFLG_IS_TRANSPARENT)
			code += "#define IS_TRANSPARENT\n";
		if (flags & MTLFLG_IS_ALPHA_MASK)
			code += "#define IS_ALPHA_MASK\n";
		if (flags & MTLFLG_IS_ALPHA_CUTOUT)
			code += "#define IS_ALPHA_CUTOUT\n";
		if (flags & MTLFLG_IS_RGB)
			code += "#define IS_RGB\n";
		if (flags & MTLFLG_IS_RGBA)
			code += "#define IS_RGBA\n";
		if (flags & MTLFLG_IS_TEXT)
			code += "#define IS_TEXT\n";
		
		code += R"GLSL(
struct ActorGpuData
{
	mat4		model;
	vec4		colorMultiplier;
	uint64_t	lightmapHandle;
	uint		flags;
	uint		ambientCubeOffset;
	uint		boneMatrixOffset;
};

struct MaterialGpuData
{
    uint textureOffset;
    uint textureCount;
    uint flags;
    float f1;
    float f2;
};

layout(std430, binding = 2) readonly buffer ActorDataBuffer
{
	ActorGpuData actors[];
};

layout(std430, binding = 3) readonly buffer MaterialDataBuffer
{
    MaterialGpuData materials[];
};

layout(std430, binding = 4) readonly buffer MaterialTextureHandleBuffer
{
    uint64_t materialTextureHandles[];
};

layout(std430, binding = 5) readonly buffer ActorAmbientCubeBuffer
{
    vec4 actorAmbientCube[];
};


in vec3			fragWorldPos;
in vec3			fragNormal;
#ifdef HAS_TEXTURES
in vec2			fragTexCoord;
#endif
#ifdef HAS_LIGHTMAP
in vec2			fragTexCoordLightMap;
#endif

flat in uint	fragActorIndex;
flat in uint	fragMaterialIndex;
flat in uint	fragActiveFrame;


#if defined(IS_TRANSPARENT) || defined(IS_ALPHA_MASK) || defined(IS_RGBA) || defined(IS_TEXT)
	layout(location = 0) out vec4 accum;
	layout(location = 1) out float reveal;
#else
	layout(location = 0) out vec4 fragColor;
#endif


vec3 sampleAmbientCube(uint offset, vec3 worldNormal)
{
    vec3 n = normalize(worldNormal);
    vec3 nSquared = n * n;

    ivec3 isNegative = ivec3(lessThan(n, vec3(0.0)));

    vec3 returnColor = nSquared.x * actorAmbientCube[offset + isNegative.x].xyz
        + nSquared.y * actorAmbientCube[offset + isNegative.y + 2].xyz
        + nSquared.z * actorAmbientCube[offset + isNegative.z + 4].xyz;

    return returnColor;
}

#if defined(IS_TRANSPARENT) || defined(IS_ALPHA_MASK) || defined(IS_RGBA) || defined(IS_TEXT)
void setTransparentOutput(vec3 color, float alpha)
{
	float weight = clamp(pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
	accum = vec4(color * alpha, alpha) * weight;
	reveal = alpha;
}
#endif

void main()
{
	const ActorGpuData actor		= actors[fragActorIndex];
	const MaterialGpuData material	= materials[fragMaterialIndex];
	vec4 color						= actor.colorMultiplier;
#ifdef HAS_TEXTURES
	const uint localTextureIndex	= min(fragActiveFrame, material.textureCount - 1u);
	const uint64_t textureHandle	= materialTextureHandles[material.textureOffset + localTextureIndex];
	sampler2D materialTexture		= sampler2D(textureHandle); // OR sampler2D(unpackUint2x32(textureHandle));
#endif
#ifdef HAS_LIGHTMAP
	sampler2D lightmapTexture = sampler2D(actor.lightmapHandle);
#endif



#ifdef IS_ALPHA_MASK

	float alpha = texture(materialTexture, fragTexCoord).a * color.a;
	
	if (alpha <= 0.001)
		discard;

	setTransparentOutput(color.rgb, alpha);

#elif defined(IS_TEXT)
	
	float alpha = texture(materialTexture, fragTexCoord).r * color.a;

	setTransparentOutput(color.rgb, alpha);

#else
	
	vec3 finalColor = vec3(1.0, 1.0, 1.0);

	#ifdef HAS_AMBIENT_CUBE

		vec3 cubeColor = sampleAmbientCube(actor.ambientCubeOffset, fragNormal);
		
		#if defined(IS_RGB) || defined(IS_RGBA)

			finalColor = cubeColor * color.rgb;

		#else

			vec4 tc = texture(materialTexture, fragTexCoord);

			#ifdef IS_ALPHA_CUTOUT
				if (tc.a < 0.99)
					discard;
			#endif

			finalColor = tc.rgb * cubeColor * color.rgb;

		#endif

	#else

		#ifdef HAS_LIGHTMAP
			
			#if defined(IS_RGB) || defined(IS_RGBA)

				finalColor = texture(lightmapTexture, fragTexCoordLightMap).rgb * color.rgb;

			#else
				
				vec4 tc = texture(materialTexture, fragTexCoord);

				#ifdef IS_ALPHA_CUTOUT
					if (tc.a < 0.99)
						discard;
				#endif

				finalColor = tc.rgb * texture(lightmapTexture, fragTexCoordLightMap).rgb * color.rgb;

			#endif

		#else
			
			#if defined(IS_RGB) || defined(IS_RGBA)

				finalColor = color.rgb;

			#else

				vec4 tc = texture(materialTexture, fragTexCoord);

				#ifdef IS_ALPHA_CUTOUT
					if (tc.a < 0.99)
						discard;
				#endif

				finalColor = tc.rgb * color.rgb;

			#endif

		#endif

	#endif

	#if defined(IS_TRANSPARENT) || defined(IS_RGBA)

		#ifdef IS_RGBA

			float alpha = color.w;

		#else

			float alpha = texture(materialTexture, fragTexCoord).a * color.a;

		#endif

		setTransparentOutput(finalColor, alpha);

	#else

		fragColor = vec4(finalColor, 1.0);

	#endif

#endif

})GLSL";
	}

	shader_handle Scene::generateShader(uint32_t flags)
	{
		if (flags & MTLFLG_IS_LINE)
			return this->generateLineShader(flags);

		std::string vertCode;
		this->generateVertexShader(flags, vertCode);

		std::string fragCode;
		this->generateFragmentShader(flags, fragCode);

		Shader s;
		Runtime::_gpu->loadShader(s, vertCode, fragCode);

		shader_handle handle = this->shaders.size();
		this->shaders.push_back(s);
		this->shaderHandleMap.emplace(flags, handle);

		return handle;
	}






} // END NAMESPACE