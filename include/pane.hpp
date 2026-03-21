#pragma once

#include <memory>

#include "mesh.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
namespace fxed {

class Pane {
	static ResourceID backgroundShaderID;
	static ResourceID backgroundMeshID;

   public:
	Pane(nri::NRI &nri, nri::CommandQueue &queue);
	void render(nri::CommandBuffer &cmdBuf);
};

}	  // namespace fxed
