#pragma once
#include <memory>
#include <cstdint>
#include <span>

#include "nri.hpp"
namespace fxed {

class Mesh {
   protected:
	std::unique_ptr<nri::Allocation> memory;

	std::unique_ptr<nri::Buffer> vertexAttributes;
	std::unique_ptr<nri::Buffer> indexBuffer;

	uint32_t vertexCount = 0;
	uint32_t indexCount	 = 0;

	void init(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
			  std::span<uint32_t> indices);

   public:
	Mesh() {}
	Mesh(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
		 std::span<uint32_t> indices);

	void bind(nri::CommandBuffer &cmdBuffer) const;
	void draw(nri::CommandBuffer &cmdBuffer, nri::GraphicsProgram &program) const;

	std::vector<nri::VertexBinding> getVertexBindings() const;
};

class TriangleMesh : public Mesh {
   public:
	TriangleMesh(nri::NRI &nri, nri::CommandQueue &q);
};

class QuadMesh : public Mesh {
   public:
	QuadMesh(nri::NRI &nri, nri::CommandQueue &q, float size = 1.0f);
};
}	  // namespace fxed
