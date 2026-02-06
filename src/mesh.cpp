#include <cstring>

#include "mesh.hpp"
#include "buffer_utils.hpp"
#include "nri.hpp"
#include "utils.hpp"

namespace fxed {
Mesh::Mesh(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
		   std::span<uint32_t> indices) {
	init(nri, q, vertices, texCoords, indices);
}

Mesh::Mesh(nri::NRI &nri, std::size_t vertexCount, std::size_t indexCount,
		   nri::MemoryTypeRequest memoryTypeRequest)
	: vertexCount(vertexCount), indexCount(indexCount) {
	initBuffers(nri, vertexCount, indexCount, memoryTypeRequest);
}

nri::MemoryRequirements Mesh::initBuffers(nri::NRI &nri, std::size_t vertexCount, std::size_t indexCount,
										  nri::MemoryTypeRequest memoryTypeRequest) {
	nri::BufferUsage bufferUsage = nri::BUFFER_USAGE_TRANSFER_DST;

	vertexAttributes = nri.createBuffer(vertexCount * (2 + 2) * sizeof(float), nri::BUFFER_USAGE_VERTEX | bufferUsage);
	indexBuffer		 = nri.createBuffer(indexCount * sizeof(uint32_t), nri::BUFFER_USAGE_INDEX | bufferUsage);

	auto [offsets, memReq] = getBufferOffsets({vertexAttributes.get(), indexBuffer.get()});
	memory				   = allocateBindMemory(nri, {vertexAttributes.get(), indexBuffer.get()}, memoryTypeRequest);
	return memReq;
}

void Mesh::init(nri::NRI &nri, nri::CommandQueue &q, std::span<float> vertices, std::span<float> texCoords,
				std::span<uint32_t> indices) {
	if (vertices.size() % 2 != 0) {
		dbLog(dbg::LOG_ERROR, "Vertices size: ", vertices.size(), " must be a multiple of 3");
		throw std::runtime_error("Vertices size must be a multiple of 3");
	}
	if (texCoords.size() % 2 != 0) {
		dbLog(dbg::LOG_ERROR, "TexCoords size: ", texCoords.size(), " must be a multiple of 2");
		throw std::runtime_error("TexCoords size must be a multiple of 2");
	}
	if (vertices.size() / 2 != texCoords.size() / 2) {
		dbLog(dbg::LOG_ERROR, "Vertices count: ", vertices.size() / 2, ", TexCoords count: ", texCoords.size() / 2,
			  " must be the same");
		throw std::runtime_error("Vertices, colors, normals and texCoords must have the same number of vertices");
	}
	if (indices.size() % 3 != 0) {
		dbLog(dbg::LOG_ERROR, "Indices size: ", indices.size(), " must be a multiple of 3");
		throw std::runtime_error("Indices size must be a multiple of 3");
	}
	vertexCount = vertices.size() / 2;
	indexCount	= indices.size();

	auto memReq = initBuffers(nri, vertexCount, indexCount, nri::MemoryTypeRequest::MEMORY_TYPE_DEVICE);

	assert(memReq.size > 0);
	auto uploadBuffer		= nri.createBuffer(memReq.size, nri::BUFFER_USAGE_TRANSFER_SRC);
	auto uploadBufferMemory = allocateBindMemory(nri, {uploadBuffer.get()}, nri::MemoryTypeRequest::MEMORY_TYPE_UPLOAD);

	{
		void	*data		   = uploadBuffer->map(0, uploadBuffer->getSize());
		uint8_t *vertexDataPtr = static_cast<uint8_t *>(data) + vertexAttributes->getOffset();
		for (std::size_t i = 0; i < vertexCount; i++) {
			std::memcpy(vertexDataPtr, &vertices[i * 2], 2 * sizeof(float));
			vertexDataPtr += 2 * sizeof(float);
			std::memcpy(vertexDataPtr, &texCoords[i * 2], 2 * sizeof(float));
			vertexDataPtr += 2 * sizeof(float);
		}
		std::memcpy(static_cast<uint8_t *>(data) + indexBuffer->getOffset(), indices.data(),
					indices.size() * sizeof(uint32_t));
		uploadBuffer->unmap();
	}

	auto cmdBuffer = nri.createCommandBuffer(nri.getDefaultCommandPool());
	vertexAttributes->copyFrom(*cmdBuffer, *uploadBuffer, vertexAttributes->getOffset(), 0,
							   vertexAttributes->getSize());
	indexBuffer->copyFrom(*cmdBuffer, *uploadBuffer, indexBuffer->getOffset(), 0, indexBuffer->getSize());

	auto key = q.submit(*cmdBuffer);
	q.wait(key);
}

void Mesh::bind(nri::CommandBuffer &cmdBuffer) const {
	vertexAttributes->bindAsVertexBuffer(cmdBuffer, 0, 0, (2 + 2) * sizeof(float));
	indexBuffer->bindAsIndexBuffer(cmdBuffer, 0, nri::IndexType::INDEX_TYPE_UINT32);
}

void Mesh::draw(nri::CommandBuffer &cmdBuffer, nri::GraphicsProgram &program) const {
	program.drawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
}

std::vector<nri::VertexBinding> Mesh::getVertexBindings() {
	return {
		{0,
		 (2 + 2) * sizeof(float),
		 nri::VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
		 {
			 {0, nri::Format::FORMAT_R32G32B32_SFLOAT, 0, nri::VertexInputRate::VERTEX_INPUT_RATE_VERTEX, "POSITION"},
			 {1, nri::Format::FORMAT_R32G32_SFLOAT, 8, nri::VertexInputRate::VERTEX_INPUT_RATE_VERTEX, "TEXCOORD0_"},
		 }}};
}

TriangleMesh::TriangleMesh(nri::NRI &nri, nri::CommandQueue &q) : Mesh() {
	std::vector<float> vertices = {
		0.0f,  0.5f,	  // Vertex 1 Position
		-0.5f, -0.5f,	  // Vertex 2 Position
		0.5f,  -0.5f,	  // Vertex 3 Position
	};
	std::vector<float> texCoords = {
		0.5f, 1.0f,		// Vertex 1 TexCoord
		0.0f, 0.0f,		// Vertex 2 TexCoord
		1.0f, 0.0f		// Vertex 3 TexCoord
	};
	std::vector<uint32_t> indices = {0, 1, 2};

	init(nri, q, vertices, texCoords, indices);
}

QuadMesh::QuadMesh(nri::NRI &nri, nri::CommandQueue &q, glm::vec2 size) {
	std::vector<float> vertices = {
		-size.x / 2, size.y / 2,	  // Vertex 1 Position
		-size.x / 2, -size.y / 2,	  // Vertex 2 Position
		size.x / 2,  -size.y / 2,	  // Vertex 3 Position
		size.x / 2,  size.y / 2,	  // Vertex 4 Position
	};

	std::vector<float> texCoords = {
		0.0f, 1.0f,		// Vertex 1 TexCoord
		0.0f, 0.0f,		// Vertex 2 TexCoord
		1.0f, 0.0f,		// Vertex 3 TexCoord
		1.0f, 1.0f		// Vertex 4 TexCoord
	};

	std::vector<uint32_t> indices = {
		0, 1, 2,	 // Triangle 1
		2, 3, 0		 // Triangle 2
	};

	init(nri, q, vertices, texCoords, indices);
}
}	  // namespace fxed
