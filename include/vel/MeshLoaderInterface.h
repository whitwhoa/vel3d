#pragma once

#include <vector>
#include <string>
#include <optional>

namespace vel
{
	class Mesh;
	class Armature;

	class MeshLoaderInterface
	{
	public:
		MeshLoaderInterface() {}
		virtual ~MeshLoaderInterface(){}

		// Tell calling class what meshes and armature exist within the file at the provided path
		virtual std::optional<std::pair<std::vector<std::string>, std::string>>
			preload(const std::string& filePath) = 0;
		
		// Calling class provides list of "loadables" which can be the exact data returned from preload,
		// or a subset of data returned from preload, as the calling class will be managing
		// the state of previously loaded elements and if this file contains any elements which have previously
		// been loaded, then we do not want to spend the time loading them from disc again
		virtual std::pair<std::vector<std::unique_ptr<Mesh>>, std::unique_ptr<Armature>> 
			load(const std::pair<const std::vector<std::string>, const std::string>& loadables) = 0;

		// Called from calling class if required
		virtual void reset() {};
	};
}