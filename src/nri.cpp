#include "nri.hpp"

namespace nri {

ResourceHandle::ResourceHandle(ResourceType type, bool writable, uint32_t index)
	: handle((type << 30) | (static_cast<uint32_t>(writable) << 29) | (index & 0x1FFFFFFF)) {}

ResourceHandle ResourceHandle::INVALID_HANDLE = ResourceHandle(0xFFFFFFFF);

ProgramBuilder &ProgramBuilder::addShaderModule(const ShaderCreateInfo &shaderInfo) {
	shaderStagesInfo.push_back(shaderInfo);
	return *this;
}
ProgramBuilder &ProgramBuilder::setVertexBindings(const std::vector<VertexBinding> &bindings) {
	vertexBindings = bindings;
	return *this;
}
ProgramBuilder &ProgramBuilder::setPrimitiveType(PrimitiveType primitiveType) {
	this->primitiveType = primitiveType;
	return *this;
}
ProgramBuilder &ProgramBuilder::setPushConstantRanges(const std::vector<PushConstantRange> &ranges) {
	pushConstantRanges = ranges;
	return *this;
}

ResourceHandle Buffer::getHandle() {
	if (handle == ResourceHandle::INVALID_HANDLE) {
		handle = createHandle();
		if (handle == ResourceHandle::INVALID_HANDLE) {
			dbLog(dbg::LOG_ERROR, "Failed to create Buffer handle! This is not fatal, but usage may fail.");
		}
	}
	return handle;
}

ResourceHandle ImageView::getHandle() {
	if (handle == ResourceHandle::INVALID_HANDLE) {
		handle = createHandle();
		if (handle == ResourceHandle::INVALID_HANDLE) {
			dbLog(dbg::LOG_ERROR, "Failed to create ImageView handle! This is not fatal, but usage may fail.");
		}
	}
	return handle;
}
}	  // namespace nri
