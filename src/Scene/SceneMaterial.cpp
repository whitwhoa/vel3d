#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	material_handle Scene::addMaterial(uint32_t flags)
	{
		Material m;
		m.flags = flags;

		auto it = this->shaderHandleMap.find(flags);
		if (it != this->shaderHandleMap.end())
		{
			SPDLOG_DEBUG("Scene::addMaterial(): Existing Shader, bypass reload: {}", flags);
			m.shaderId = this->shaders[it->second].programId;
		}
		else
		{
			SPDLOG_DEBUG("Scene::addMaterial(): Loading new Shader: {}", flags);
			m.shaderId = this->shaders[this->generateShader(flags)].programId;
		}		

		material_handle handle = this->materials.size();
		this->materials.push_back(m);
	}

	shader_handle Scene::generateShader(uint32_t flags)
	{
		// TODO: implement shader program generation logic
		// IE, use flag values to determine how we should build vertex/geometry/fragment shaders
	}


} // END NAMESPACE