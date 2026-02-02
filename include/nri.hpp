#pragma once

#include <cstddef>
#include <memory>
#include <glm/glm.hpp>
#include <iostream>

#include "timer.hpp"
#include "utils.hpp"

struct GLFWwindow;

namespace nri {

class Buffer;
class Allocation;
class Image2D;
class CommandPool;
class CommandQueue;
class CommandBuffer;
class Window;
class Program;
class GraphicsProgram;
class ComputeProgram;
class RayTracingProgram;
class ProgramBuilder;
class BLAS;
class TLAS;
class Renderer;

class ResourceHandle;

class CreateBits {
   public:
	enum CreateBitsE { DEFAULT = 0, GLFW = 1 << 0, QT = 1 << 1 };
	CreateBitsE bits;
	CreateBits(CreateBitsE bits = DEFAULT) : bits(bits) {}
				operator int() const { return static_cast<int>(bits); }
	CreateBitsE operator|(CreateBitsE other) const {
		return CreateBitsE(static_cast<int>(bits) | static_cast<int>(other));
	}
	CreateBitsE operator&(CreateBitsE other) const {
		return CreateBitsE(static_cast<int>(bits) & static_cast<int>(other));
	}
	operator bool() const { return bits != DEFAULT; }
};

enum BufferUsage {
	BUFFER_USAGE_VERTEX					= 1 << 0,
	BUFFER_USAGE_INDEX					= 1 << 1,
	BUFFER_USAGE_UNIFORM				= 1 << 2,
	BUFFER_USAGE_STORAGE				= 1 << 3,
	BUFFER_USAGE_TRANSFER_SRC			= 1 << 4,
	BUFFER_USAGE_TRANSFER_DST			= 1 << 5,
	BUFFER_USAGE_ACCELERATION_STRUCTURE = 1 << 6,
	BUFFER_USAGE_SHADER_BINDING_TABLE	= 1 << 7
};

enum Format {
	FORMAT_UNDEFINED			= 0,
	FORMAT_R4G4_UNORM			= 1,
	FORMAT_R4G4B4A4_UNORM		= 2,
	FORMAT_B4G4R4A4_UNORM		= 3,
	FORMAT_R5G6B5_UNORM			= 4,
	FORMAT_B5G6R5_UNORM			= 5,
	FORMAT_R5G5B5A1_UNORM		= 6,
	FORMAT_B5G5R5A1_UNORM		= 7,
	FORMAT_A1R5G5B5_UNORM		= 8,
	FORMAT_R8_UNORM				= 9,
	FORMAT_R8_SNORM				= 10,
	FORMAT_R8_USCALED			= 11,
	FORMAT_R8_SSCALED			= 12,
	FORMAT_R8_UINT				= 13,
	FORMAT_R8_SINT				= 14,
	FORMAT_R8_SRGB				= 15,
	FORMAT_R8G8_UNORM			= 16,
	FORMAT_R8G8_SNORM			= 17,
	FORMAT_R8G8_USCALED			= 18,
	FORMAT_R8G8_SSCALED			= 19,
	FORMAT_R8G8_UINT			= 20,
	FORMAT_R8G8_SINT			= 21,
	FORMAT_R8G8_SRGB			= 22,
	FORMAT_R8G8B8_UNORM			= 23,
	FORMAT_R8G8B8_SNORM			= 24,
	FORMAT_R8G8B8_USCALED		= 25,
	FORMAT_R8G8B8_SSCALED		= 26,
	FORMAT_R8G8B8_UINT			= 27,
	FORMAT_R8G8B8_SINT			= 28,
	FORMAT_R8G8B8_SRGB			= 29,
	FORMAT_B8G8R8_UNORM			= 30,
	FORMAT_B8G8R8_SNORM			= 31,
	FORMAT_B8G8R8_USCALED		= 32,
	FORMAT_B8G8R8_SSCALED		= 33,
	FORMAT_B8G8R8_UINT			= 34,
	FORMAT_B8G8R8_SINT			= 35,
	FORMAT_B8G8R8_SRGB			= 36,
	FORMAT_R8G8B8A8_UNORM		= 37,
	FORMAT_R8G8B8A8_SNORM		= 38,
	FORMAT_R8G8B8A8_USCALED		= 39,
	FORMAT_R8G8B8A8_SSCALED		= 40,
	FORMAT_R8G8B8A8_UINT		= 41,
	FORMAT_R8G8B8A8_SINT		= 42,
	FORMAT_R8G8B8A8_SRGB		= 43,
	FORMAT_B8G8R8A8_UNORM		= 44,
	FORMAT_B8G8R8A8_SNORM		= 45,
	FORMAT_B8G8R8A8_USCALED		= 46,
	FORMAT_B8G8R8A8_SSCALED		= 47,
	FORMAT_B8G8R8A8_UINT		= 48,
	FORMAT_B8G8R8A8_SINT		= 49,
	FORMAT_B8G8R8A8_SRGB		= 50,
	FORMAT_A8B8G8R8_UNORM		= 51,
	FORMAT_A8B8G8R8_SNORM		= 52,
	FORMAT_A8B8G8R8_USCALED		= 53,
	FORMAT_A8B8G8R8_SSCALED		= 54,
	FORMAT_A8B8G8R8_UINT		= 55,
	FORMAT_A8B8G8R8_SINT		= 56,
	FORMAT_A8B8G8R8_SRGB		= 57,
	FORMAT_A2R10G10B10_UNORM	= 58,
	FORMAT_A2R10G10B10_SNORM	= 59,
	FORMAT_A2R10G10B10_USCALED	= 60,
	FORMAT_A2R10G10B10_SSCALED	= 61,
	FORMAT_A2R10G10B10_UINT		= 62,
	FORMAT_A2R10G10B10_SINT		= 63,
	FORMAT_A2B10G10R10_UNORM	= 64,
	FORMAT_A2B10G10R10_SNORM	= 65,
	FORMAT_A2B10G10R10_USCALED	= 66,
	FORMAT_A2B10G10R10_SSCALED	= 67,
	FORMAT_A2B10G10R10_UINT		= 68,
	FORMAT_A2B10G10R10_SINT		= 69,
	FORMAT_R16_UNORM			= 70,
	FORMAT_R16_SNORM			= 71,
	FORMAT_R16_USCALED			= 72,
	FORMAT_R16_SSCALED			= 73,
	FORMAT_R16_UINT				= 74,
	FORMAT_R16_SINT				= 75,
	FORMAT_R16_SFLOAT			= 76,
	FORMAT_R16G16_UNORM			= 77,
	FORMAT_R16G16_SNORM			= 78,
	FORMAT_R16G16_USCALED		= 79,
	FORMAT_R16G16_SSCALED		= 80,
	FORMAT_R16G16_UINT			= 81,
	FORMAT_R16G16_SINT			= 82,
	FORMAT_R16G16_SFLOAT		= 83,
	FORMAT_R16G16B16_UNORM		= 84,
	FORMAT_R16G16B16_SNORM		= 85,
	FORMAT_R16G16B16_USCALED	= 86,
	FORMAT_R16G16B16_SSCALED	= 87,
	FORMAT_R16G16B16_UINT		= 88,
	FORMAT_R16G16B16_SINT		= 89,
	FORMAT_R16G16B16_SFLOAT		= 90,
	FORMAT_R16G16B16A16_UNORM	= 91,
	FORMAT_R16G16B16A16_SNORM	= 92,
	FORMAT_R16G16B16A16_USCALED = 93,
	FORMAT_R16G16B16A16_SSCALED = 94,
	FORMAT_R16G16B16A16_UINT	= 95,
	FORMAT_R16G16B16A16_SINT	= 96,
	FORMAT_R16G16B16A16_SFLOAT	= 97,
	FORMAT_R32_UINT				= 98,
	FORMAT_R32_SINT				= 99,
	FORMAT_R32_SFLOAT			= 100,
	FORMAT_R32G32_UINT			= 101,
	FORMAT_R32G32_SINT			= 102,
	FORMAT_R32G32_SFLOAT		= 103,
	FORMAT_R32G32B32_UINT		= 104,
	FORMAT_R32G32B32_SINT		= 105,
	FORMAT_R32G32B32_SFLOAT		= 106,
	FORMAT_R32G32B32A32_UINT	= 107,
	FORMAT_R32G32B32A32_SINT	= 108,
	FORMAT_R32G32B32A32_SFLOAT	= 109,
	FORMAT_R64_UINT				= 110,
	FORMAT_R64_SINT				= 111,
	FORMAT_R64_SFLOAT			= 112,
	FORMAT_R64G64_UINT			= 113,
	FORMAT_R64G64_SINT			= 114,
	FORMAT_R64G64_SFLOAT		= 115,
	FORMAT_R64G64B64_UINT		= 116,
	FORMAT_R64G64B64_SINT		= 117,
	FORMAT_R64G64B64_SFLOAT		= 118,
	FORMAT_R64G64B64A64_UINT	= 119,
	FORMAT_R64G64B64A64_SINT	= 120,
	FORMAT_R64G64B64A64_SFLOAT	= 121,
	FORMAT_B10G11R11_UFLOAT		= 122,
	FORMAT_E5B9G9R9_UFLOAT		= 123,
	FORMAT_D16_UNORM			= 124,
	FORMAT_X8_D24_UNORM			= 125,
	FORMAT_D32_SFLOAT			= 126,
	FORMAT_S8_UINT				= 127,
	FORMAT_D16_UNORM_S8_UINT	= 128,
	FORMAT_D24_UNORM_S8_UINT	= 129,
	FORMAT_D32_SFLOAT_S8_UINT	= 130,
	_FORMAT_NUM					= 131
};

enum MemoryTypeRequest {
	MEMORY_TYPE_UPLOAD	 = 0,
	MEMORY_TYPE_READBACK = 1,
	MEMORY_TYPE_DEVICE	 = 2,
	_MEMORY_TYPE_NUM	 = 3
};

enum ImageUsage {
	IMAGE_USAGE_TRANSFER_SRC			 = 1 << 0,
	IMAGE_USAGE_TRANSFER_DST			 = 1 << 1,
	IMAGE_USAGE_SAMPLED					 = 1 << 2,
	IMAGE_USAGE_STORAGE					 = 1 << 3,
	IMAGE_USAGE_COLOR_ATTACHMENT		 = 1 << 4,
	IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 5,
	IMAGE_USAGE_TRANSIENT_ATTACHMENT	 = 1 << 6,
	IMAGE_USAGE_INPUT_ATTACHMENT		 = 1 << 7
};

enum ShaderType {
	SHADER_TYPE_VERTEX					= 0,
	SHADER_TYPE_FRAGMENT				= 1,
	SHADER_TYPE_COMPUTE					= 2,
	SHADER_TYPE_GEOMETRY				= 3,
	SHADER_TYPE_TESSELLATION_CONTROL	= 4,
	SHADER_TYPE_TESSELLATION_EVALUATION = 5,
	SHADER_TYPE_RAYGEN					= 6,
	SHADER_TYPE_ANY_HIT					= 7,
	SHADER_TYPE_CLOSEST_HIT				= 8,
	SHADER_TYPE_MISS					= 9,
	SHADER_TYPE_INTERSECTION			= 10,
	SHADER_TYPE_MESH					= 11,
	SHADER_TYPE_TASK					= 12,
	_SHADER_TYPE_NUM
};

struct MemoryRequirements {
	std::size_t		  size;
	std::size_t		  alignment = 0;
	MemoryTypeRequest typeRequest;

	MemoryRequirements(std::size_t s, MemoryTypeRequest tr, std::size_t a = 0)
		: size(s), alignment(a), typeRequest(tr) {}

	MemoryRequirements &setTypeRequest(MemoryTypeRequest tr);
	MemoryRequirements &setAlignment(std::size_t a);
};

struct ShaderCreateInfo {
	std::string sourceFile;		/// path to the shader source file
	std::string entryPoint;		/// entry point function name
	ShaderType	shaderType;		/// type of the shader (vertex, fragment, etc.)
};

enum VertexInputRate { VERTEX_INPUT_RATE_VERTEX = 0, VERTEX_INPUT_RATE_INSTANCE = 1 };

struct VertexAttribute {
	uint32_t		location;	   /// shader-specified location
	Format			format;		   /// format of the attribute
	uint32_t		offset;		   /// offset in vertex buffer
	VertexInputRate inputRate;	   /// input rate (per-vertex or per-instance)
	std::string		name;		   /// optional name of the attribute
};

struct VertexBinding {
	uint32_t					 binding;		/// binding index
	uint32_t					 stride;		/// size of one vertex
	VertexInputRate				 inputRate;		/// input rate (per-vertex or per-instance)
	std::vector<VertexAttribute> attributes;
};

enum PrimitiveType {
	PRIMITIVE_TYPE_TRIANGLES	  = 0,
	PRIMITIVE_TYPE_TRIANGLE_STRIP = 1,
	PRIMITIVE_TYPE_LINES		  = 2,
	PRIMITIVE_TYPE_LINE_STRIP	  = 3,
	PRIMITIVE_TYPE_POINTS		  = 4
};

enum IndexType { INDEX_TYPE_UINT16 = 0, INDEX_TYPE_UINT32 = 1, _INDEX_TYPE_NUM = 2 };

struct PushConstantRange {
	uint32_t offset;
	uint32_t size;
};

/// NRI - Native Rendering Interface
class NRI {
   public:
	virtual ~NRI() {}
	NRI(CreateBits) {}

	virtual std::unique_ptr<Buffer>	 createBuffer(std::size_t size, BufferUsage usage)							  = 0;
	virtual std::unique_ptr<Image2D> createImage2D(uint32_t width, uint32_t height, Format fmt, ImageUsage usage) = 0;
	virtual std::unique_ptr<Allocation>	   allocateMemory(MemoryRequirements memoryRequirements)				  = 0;
	virtual std::unique_ptr<CommandQueue>  createCommandQueue()													  = 0;
	virtual std::unique_ptr<CommandBuffer> createCommandBuffer(const CommandPool &commandPool)					  = 0;
	virtual std::unique_ptr<CommandPool>   createCommandPool()													  = 0;
	virtual CommandPool					  &getDefaultCommandPool()												  = 0;

	virtual std::unique_ptr<ProgramBuilder> createProgramBuilder() = 0;
	virtual std::unique_ptr<Window> createGLFWWindow(GLFWwindow *w) = 0;

	virtual std::unique_ptr<BLAS> createBLAS(Buffer &vertexBuffer, Format vertexFormat, std::size_t vertexOffset,
											 uint32_t vertexCount, std::size_t vertexStride, Buffer &indexBuffer,
											 IndexType indexType, std::size_t indexOffset)					  = 0;
	virtual std::unique_ptr<TLAS> createTLAS(const std::span<const BLAS *>		  &blases,
											 std::optional<std::span<glm::mat3x4>> transforms = std::nullopt) = 0;

	virtual bool shouldFlipY() const		= 0;
	virtual bool supportsRayTracing() const = 0;
	virtual bool supportsTextures() const	= 0;

	virtual void synchronize() const = 0;
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
	return static_cast<BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr ImageUsage operator|(ImageUsage a, ImageUsage b) {
	return static_cast<ImageUsage>(static_cast<int>(a) | static_cast<int>(b));
}

enum ResourceType {
	RESOURCE_TYPE_IMAGE_SAMPLER = 0,
	RESOURCE_TYPE_STORAGE_IMAGE = 1,
	RESOURCE_TYPE_BUFFER		= 2,
	RESOURCE_TYPE_TLAS			= 3,
};

/// https://vulkan.org/user/pages/09.events/vulkanised-2023/vulkanised_2023_setting_up_a_bindless_rendering_pipeline.pdf
/// 2 bits for resource type,
/// 1 bit for writability,
/// ?? bits for versions??
/// 29 bits for index within the type -> too much
class ResourceHandle final {
	uint32_t handle;

	ResourceHandle(uint32_t h) : handle(h) {}

   public:
	ResourceHandle(ResourceType type, bool writable, uint32_t index);
	ResourceHandle() {}

	ResourceType getType() const;
	bool		 isWritable() const;
	uint32_t	 getIndex() const { return handle & 0x1FFFFFFF; }

	static ResourceHandle INVALID_HANDLE;

	bool operator==(const ResourceHandle &other) const	   = default;
	bool operator!=(const ResourceHandle &other) const	   = default;
	ResourceHandle(const ResourceHandle &other)			   = default;
	ResourceHandle &operator=(const ResourceHandle &other) = default;
};

class Allocation {
   public:
	virtual ~Allocation() {}
	virtual std::size_t getSize() const = 0;
};

class Buffer {
   protected:
	ResourceHandle		   handle				= ResourceHandle::INVALID_HANDLE;
	virtual ResourceHandle createHandle() const = 0;

   public:
	virtual ~Buffer() {}

	virtual MemoryRequirements getMemoryRequirements()								  = 0;
	virtual void			   bindMemory(Allocation &allocation, std::size_t offset) = 0;
	virtual void			  *map(std::size_t offset, std::size_t size)			  = 0;
	virtual void			   unmap()												  = 0;
	virtual std::size_t		   getSize() const										  = 0;
	virtual std::size_t		   getOffset() const									  = 0;

	virtual void copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset, std::size_t dstOffset,
						  std::size_t size) = 0;

	virtual void bindAsVertexBuffer(CommandBuffer &commandBuffer, uint32_t binding, std::size_t offset,
									std::size_t stride)													  = 0;
	virtual void bindAsIndexBuffer(CommandBuffer &commandBuffer, std::size_t offset, IndexType indexType) = 0;

	ResourceHandle getHandle();
};

class ImageView {
   protected:
	ResourceHandle		   handle;
	virtual ResourceHandle createHandle() const = 0;

   public:
	ImageView() : handle(ResourceHandle::INVALID_HANDLE) {}
	virtual ~ImageView() {}

	ResourceHandle getHandle();
};

class Image2D {
   public:
	virtual ~Image2D() {}

	virtual MemoryRequirements getMemoryRequirements()								  = 0;
	virtual void			   bindMemory(Allocation &allocation, std::size_t offset) = 0;

	virtual void clear(CommandBuffer &commandBuffer, glm::vec4 color) = 0;
	virtual void prepareForPresent(CommandBuffer &commandBuffer)	  = 0;
	virtual void prepareForStorage(CommandBuffer &commandBuffer)	  = 0;
	virtual void prepareForTexture(CommandBuffer &commandBuffer)	  = 0;

	virtual void copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
						  uint32_t srcRowPitch) = 0;

	virtual uint32_t getWidth() const  = 0;
	virtual uint32_t getHeight() const = 0;

	virtual std::unique_ptr<ImageView> createRenderTargetView() = 0;
	virtual std::unique_ptr<ImageView> createTextureView()		= 0;
	virtual std::unique_ptr<ImageView> createStorageView()		= 0;
};

class CommandPool {
   public:
	virtual ~CommandPool() {}
};

class CommandQueue {
   public:
	virtual ~CommandQueue() {}

	using SubmitKey = uint64_t;

	virtual SubmitKey submit(CommandBuffer &commandBuffer) = 0;
	virtual void	  wait(SubmitKey)					   = 0;
};

class CommandBuffer {
   public:
	virtual ~CommandBuffer() {}

	virtual void begin() = 0;
	virtual void end()	 = 0;
};

class ProgramBuilder {
   protected:
	std::vector<ShaderCreateInfo>  shaderStagesInfo;
	std::vector<VertexBinding>	   vertexBindings;
	PrimitiveType				   primitiveType;
	std::vector<PushConstantRange> pushConstantRanges;

   public:
	virtual ~ProgramBuilder() {}

	ProgramBuilder							  &addShaderModule(const ShaderCreateInfo &shaderInfo);
	ProgramBuilder							  &setVertexBindings(const std::vector<VertexBinding> &bindings);
	ProgramBuilder							  &setPrimitiveType(PrimitiveType primitiveType);
	ProgramBuilder							  &setPushConstantRanges(const std::vector<PushConstantRange> &ranges);
	virtual std::unique_ptr<GraphicsProgram>   buildGraphicsProgram()							  = 0;
	virtual std::unique_ptr<ComputeProgram>	   buildComputeProgram()							  = 0;
	virtual std::unique_ptr<RayTracingProgram> buildRayTracingProgram(nri::CommandBuffer &cmdBuf) = 0;
};

/// Program - represents a GPU program (shader)
/// should contain shader modules, pipeline state and layout
class Program {
   public:
	virtual ~Program() {}

	virtual void bind(CommandBuffer &commandBuffer)	  = 0;
	virtual void unbind(CommandBuffer &commandBuffer) = 0;

	virtual void setPushConstants(CommandBuffer &commandBuffer, const void *data, std::size_t size,
								  std::size_t offset) = 0;
};

class GraphicsProgram : virtual public Program {
   public:
	virtual void draw(CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
					  uint32_t firstInstance) = 0;

	virtual void drawIndexed(CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount,
							 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
};

class ComputeProgram : virtual public Program {
   public:
	virtual void dispatch(CommandBuffer &commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
						  uint32_t groupCountZ) = 0;
};

class RayTracingProgram : virtual public Program {
   public:
	virtual void traceRays(CommandBuffer &commandBuffer, uint32_t width, uint32_t height, uint32_t depth) = 0;
};

// Bottom-Level Acceleration Structure (BLAS)
class BLAS {
   public:
	virtual void build(CommandBuffer &commandBuffer) = 0;
	virtual void buildFinished() {}

	virtual ~BLAS() {}
};

class TLAS {
   public:
	virtual void build(CommandBuffer &commandBuffer) = 0;
	virtual void buildFinished() {}

	virtual ResourceHandle getHandle() const = 0;

	virtual ~TLAS() {}
};

template <class ImageType, class ImageViewType>
class ImageAndView {
   public:
	ImageType	  image;
	ImageViewType view;
};

class ImageAndViewPtr {
   public:
	std::unique_ptr<Image2D>   image;
	std::unique_ptr<ImageView> view;
};

class ImageAndViewRef {
   public:
	Image2D	  &image;
	ImageView &view;
};

// class Renderer : public QWidget {
//    protected:
//	NRI &nri;
//
//    public:
//	Renderer(NRI &nri) : nri(nri) {}
//	virtual void initialize(QWindow &window)							   = 0;
//	virtual void render(const ImageAndViewRef &img, CommandBuffer &cmdBuf) = 0;
//	virtual ~Renderer() {}
// };

class Window {
   protected:
	NRI &nri;

   public:
	virtual ~Window() {}
	Window(NRI &nri) : nri(nri) {}

	virtual void			beginFrame()			 = 0;
	virtual void			endFrame()				 = 0;
	virtual ImageAndViewRef getCurrentRenderTarget() = 0;

	virtual void beginRendering(CommandBuffer &cmdBuf, const ImageAndViewRef &renderTarget) = 0;
	virtual void endRendering(CommandBuffer &cmdBuf)										= 0;

	virtual CommandQueue &getMainQueue() = 0;
};

}	  // namespace nri
