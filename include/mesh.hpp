#pragma once
#include <memory>
#include <cstdint>
#include <span>

#include "nri.hpp"
namespace fxed {

class Mesh {
   protected:
	struct Vertex {
		float x, y;
		float u, v;
	};

	std::unique_ptr<nri::Allocation> memory;

	std::unique_ptr<nri::Buffer> vertexAttributes;
	std::unique_ptr<nri::Buffer> indexBuffer;

	uint32_t vertexCount = 0;
	uint32_t indexCount	 = 0;

	void init(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
			  std::span<uint32_t> indices);
	nri::MemoryRequirements initBuffers(
		nri::NRI &nri, std::size_t vertexCount, std::size_t indexCount,
		nri::MemoryTypeRequest memoryTypeRequest = nri::MemoryTypeRequest::MEMORY_TYPE_DEVICE);

   public:
	Mesh() {}
	Mesh(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
		 std::span<uint32_t> indices);
	Mesh(nri::NRI &nri, std::size_t vertexCount, std::size_t indexCount,
		 nri::MemoryTypeRequest memoryTypeRequest = nri::MemoryTypeRequest::MEMORY_TYPE_DEVICE);

	void bind(nri::CommandBuffer &cmdBuffer) const;
	void draw(nri::CommandBuffer &cmdBuffer, nri::GraphicsProgram &program) const;

	static std::vector<nri::VertexBinding> getVertexBindings();
};

class TriangleMesh : public Mesh {
   public:
	TriangleMesh(nri::NRI &nri, nri::CommandQueue &q);
};

class QuadMesh : public Mesh {
   public:
	QuadMesh(nri::NRI &nri, nri::CommandQueue &q, glm::vec2 size = glm::vec2(1.0f, 1.0f));
};
}	  // namespace fxed
