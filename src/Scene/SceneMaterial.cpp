#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	material_handle Scene::addMaterial(uint32_t flags)
	{
		Material m;
		m.flags = flags;
		m.shaderProgramId = this->getShaderProgramId(flags);
	
		material_handle handle = this->materials.size();
		this->materials.push_back(m);

		return handle;
	}

} // END NAMESPACE