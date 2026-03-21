#pragma once

#include <memory>

#include "mesh.hpp"
#include "nri.hpp"
namespace fxed {

class Pane {
	static std::unique_ptr<nri::GraphicsProgram> backgroundShader;
	static std::unique_ptr<fxed::QuadMesh>		 backgroundMesh;

   public:
	Pane(nri::NRI &nri, nri::CommandQueue &queue);
	void render(nri::CommandBuffer &cmdBuf);
};

}	  // namespace fxed
