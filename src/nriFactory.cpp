#include "nriFactory.hpp"
#include <format>

namespace nri {
Factory &Factory::getInstance() {
	static Factory instance;
	return instance;
}

void Factory::registerNRI(const char *name, CreateFunction createFunction) { nriInfos.emplace(name, createFunction); }

std::unique_ptr<NRI> Factory::createNRI(const std::string &name, CreateBits createBits) const {
	auto it = nriInfos.find(name);
	if (it != nriInfos.end()) {
		return it->second(createBits);
	} else throw std::runtime_error(std::format("Native Rendering Interface not found: {}", name));
	return nullptr;
}

std::vector<std::string> Factory::getAvailableNRIs() const {
	std::vector<std::string> names;
	for (const auto &pair : nriInfos) {
		names.push_back(pair.first);
	}
	return names;
}

}	  // namespace nri
