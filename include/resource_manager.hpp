#pragma once

#include <queue>
#include <type_traits>
#include "mesh.hpp"
#include "nri.hpp"

namespace fxed {

class ResourceID {
	uint32_t id;

   public:
	constexpr ResourceID(uint32_t id) : id(id) {}
	constexpr ResourceID() : id(UINT32_MAX) {}
	constexpr ResourceID(const ResourceID &)			= default;
	constexpr ResourceID &operator=(const ResourceID &) = default;

	static ResourceID invalid() { return ResourceID{UINT32_MAX}; }
	bool			  isValid() const { return id != UINT32_MAX; }
	bool			  operator==(const ResourceID &other) const { return id == other.id; }
	bool			  operator!=(const ResourceID &other) const { return id != other.id; }
					  operator uint32_t() const { return id; }

	template <typename T>
		requires std::is_class_v<T>
	constexpr operator T &();
};

template <typename T>
class Pool {
	std::vector<std::unique_ptr<T>> resources;
	std::queue<ResourceID>			freeIDs;

   public:
	ResourceID add(std::unique_ptr<T> resource) {
		if (!freeIDs.empty()) {
			ResourceID id = freeIDs.front();
			freeIDs.pop();
			resources[id] = std::move(resource);
			return id;
		} else {
			resources.push_back(std::move(resource));
			return resources.size() - 1;
		}
	}

	T &get(ResourceID id) {
		if (id >= resources.size() || !resources[id])
			throw std::runtime_error("ResourceID out of range or resource deleted");
		return *resources[id];
	}
};

class ResourceManager {
	Pool<fxed::Mesh>		   meshes;
	Pool<nri::GraphicsProgram> shaders;

	ResourceManager() = default;
	DELETE_COPY_AND_ASSIGNMENT(ResourceManager);
	ResourceManager(ResourceManager &&)			   = delete;
	ResourceManager &operator=(ResourceManager &&) = delete;

   public:
	ResourceID	addMesh(std::unique_ptr<fxed::Mesh> &&mesh) { return meshes.add(std::move(mesh)); }
	fxed::Mesh &getMesh(ResourceID id) { return meshes.get(id); }

	ResourceID addShader(std::unique_ptr<nri::GraphicsProgram> &&shader) { return shaders.add(std::move(shader)); }
	nri::GraphicsProgram &getShader(ResourceID id) { return shaders.get(id); }

	template <typename T>
	T &get(ResourceID id);

	template <>
	fxed::Mesh &get<fxed::Mesh>(ResourceID id) {
		return getMesh(id);
	}

	template <>
	nri::GraphicsProgram &get<nri::GraphicsProgram>(ResourceID id) {
		return getShader(id);
	}

	static ResourceManager &getInstance() {
		static ResourceManager instance;
		return instance;
	}
};

template <typename T>
	requires std::is_class_v<T>
constexpr ResourceID::operator T &() {
	return ResourceManager::getInstance().get<T>(*this);
}

};	   // namespace fxed
