#include <spdlog/spdlog.h>

#include <vel/Scene/Mesh/Mesh.h>


namespace vel
{
    Mesh::Mesh(const std::string& name) :
        name(name)
    {}

	void Mesh::refreshAABB()
	{
		// TODO: implement
	}

	MeshBone* Mesh::getBone(const std::string& boneName)
	{
		for (auto& b : this->bones)
		{
			if (b.name == boneName)
				return &b;
		}
		return nullptr;
	}
}