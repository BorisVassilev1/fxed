#pragma once

#include <map>
#include <functional>

#include "nri.hpp"

namespace nri {

class Factory {
	using CreateFunction = std::function<std::unique_ptr<NRI>(CreateBits bits)>;

	std::map<std::string, CreateFunction> nriInfos;

   public:
	Factory() = default;

	static Factory &getInstance();

	void					 registerNRI(const char *name, CreateFunction createFunction);
	std::unique_ptr<NRI>	 createNRI(const std::string &name, CreateBits createBits) const;
	std::vector<std::string> getAvailableNRIs() const;
};

}	  // namespace nri
