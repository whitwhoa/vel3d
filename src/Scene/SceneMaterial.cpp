#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	unsigned int Scene::addMaterial(uint32_t flags)
	{
		Material m;
		m.flags = flags;

		auto it = this->shaders.begin();
		for (; it != this->shaders.end(); ++it)
		{
			if (it->materialFlags == flags)
			{
				SPDLOG_DEBUG("Scene::addMaterial(): Existing Shader, bypass reload: {}", flags);

				m.shaderId = it->id;
				break;
			}
		}

		if (it == this->shaders.end())
		{
			SPDLOG_DEBUG("Scene::addMaterial(): Loading new Shader: {}", flags);

			m.shaderId = this->generateShader(flags);
		}
		

		unsigned int materialIndex = this->materials.size();
		this->materials.push_back(m);

		// TODO: finish implementing...the gpu data part

	}

	unsigned int Scene::generateShader(uint32_t flags)
	{
		// TODO: implement shader program generation logic
	}


} // END NAMESPACE