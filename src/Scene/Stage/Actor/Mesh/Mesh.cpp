#include <spdlog/spdlog.h>

#include <vel/Scene/Stage/Actor/Mesh/Mesh.h>


namespace vel
{
    Mesh::Mesh(std::string name) :
        name(name)
    {}

	void Mesh::refreshAABB()
	{
		// TODO: implement
	}

	MeshBone* Mesh::getBone(std::string boneName)
	{
		for (auto& b : this->bones)
		{
			if (b.name == boneName)
				return &b;
		}
		return nullptr;
	}

	bool Mesh::isRenderable() const
	{
		return this->gp->gpuGeoPool.has_value();
	}

}