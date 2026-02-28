#pragma once

#include "nri.hpp"
#include "utils.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace vkraii {
class SurfaceKHR {
	vk::SurfaceKHR surface;
	vk::Instance   instance;

   public:
	void clear() {
		if (surface) {
			dbLog(dbg::LOG_INFO, "Destroying surface ", (void *)surface);
			vkDestroySurfaceKHR(instance, surface, nullptr);
			surface = nullptr;
		}
	}

	SurfaceKHR(std::nullptr_t) : surface(nullptr), instance(nullptr) {}
	SurfaceKHR(vk::Instance instance, vk::SurfaceKHR surface) : surface(surface), instance(instance) {}
	~SurfaceKHR() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(SurfaceKHR);
	SurfaceKHR(SurfaceKHR &&other) noexcept : surface(other.surface), instance(other.instance) {
		other.surface = nullptr;
	}
	SurfaceKHR &operator=(SurfaceKHR &&other) noexcept {
		if (this != &other) {
			clear();
			surface		  = other.surface;
			instance	  = other.instance;
			other.surface = nullptr;
		}
		return *this;
	}

						  operator vk::SurfaceKHR() const { return surface; }
						  operator VkSurfaceKHR() const { return surface; }
	const vk::SurfaceKHR &operator*() const { return surface; }
	vk::SurfaceKHR		 &operator*() { return surface; }
};

class ImageView {
	vk::ImageView imageView;
	vk::Device	  device;

   public:
	void clear() {
		if (imageView) {
			vkDestroyImageView(device, imageView, nullptr);
			imageView = nullptr;
		}
	}

	ImageView(std::nullptr_t) : imageView(nullptr), device(nullptr) {}
	ImageView(vk::Device device, vk::ImageView imageView) : imageView(imageView), device(device) {}
	~ImageView() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(ImageView);
	ImageView(ImageView &&other) noexcept : imageView(other.imageView), device(other.device) {
		other.imageView = nullptr;
	}
	ImageView &operator=(ImageView &&other) noexcept {
		if (this != &other) {
			clear();
			imageView		= other.imageView;
			device			= other.device;
			other.imageView = nullptr;
		}
		return *this;
	}

						 operator vk::ImageView() const { return imageView; }
						 operator VkImageView() const { return imageView; }
	const vk::ImageView &operator*() const { return imageView; }
	vk::ImageView		&operator*() { return imageView; }
};

class Image {
	vk::Image  image;
	vk::Device device;

   public:
	void clear() {
		if (image) {
			vkDestroyImage(device, image, nullptr);
			image = nullptr;
		}
	}

	Image(std::nullptr_t) : image(nullptr), device(nullptr) {}
	Image(vk::Device device, vk::Image image) : image(image), device(device) {}
	~Image() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Image);
	Image(Image &&other) noexcept : image(other.image), device(other.device) { other.image = nullptr; }
	Image &operator=(Image &&other) noexcept {
		if (this != &other) {
			clear();
			image		= other.image;
			device		= other.device;
			other.image = nullptr;
		}
		return *this;
	}

					 operator vk::Image() const { return image; }
					 operator VkImage() const { return image; }
	const vk::Image &operator*() const { return image; }
	vk::Image		&operator*() { return image; }
};

class Buffer {
	vk::Buffer buffer;
	vk::Device device;

   public:
	void clear() {
		if (buffer) {
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = nullptr;
		}
	}

	Buffer(std::nullptr_t) : buffer(nullptr), device(nullptr) {}
	Buffer(vk::Device device, vk::Buffer buffer) : buffer(buffer), device(device) {}
	~Buffer() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Buffer);
	Buffer(Buffer &&other) noexcept : buffer(other.buffer), device(other.device) { other.buffer = nullptr; }
	Buffer &operator=(Buffer &&other) noexcept {
		if (this != &other) {
			clear();
			buffer		 = other.buffer;
			device		 = other.device;
			other.buffer = nullptr;
		}
		return *this;
	}

					  operator vk::Buffer() const { return buffer; }
					  operator VkBuffer() const { return buffer; }
	const vk::Buffer &operator*() const { return buffer; }
	vk::Buffer		 &operator*() { return buffer; }
};

class DescriptorPool {
	vk::DescriptorPool descriptorPool;
	vk::Device		   device;

   public:
	void clear() {
		if (descriptorPool) {
			dbLog(dbg::LOG_INFO, "Destroying descriptor pool ", (void *)descriptorPool);
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
			descriptorPool = nullptr;
		}
	}

	DescriptorPool(std::nullptr_t) : descriptorPool(nullptr), device(nullptr) {}
	DescriptorPool(vk::Device device, vk::DescriptorPool descriptorPool)
		: descriptorPool(descriptorPool), device(device) {}
	~DescriptorPool() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(DescriptorPool);
	DescriptorPool(DescriptorPool &&other) noexcept : descriptorPool(other.descriptorPool), device(other.device) {
		other.descriptorPool = nullptr;
	}
	DescriptorPool &operator=(DescriptorPool &&other) noexcept {
		if (this != &other) {
			clear();
			descriptorPool		 = other.descriptorPool;
			device				 = other.device;
			other.descriptorPool = nullptr;
		}
		return *this;
	}

							  operator vk::DescriptorPool() const { return descriptorPool; }
							  operator VkDescriptorPool() const { return descriptorPool; }
	const vk::DescriptorPool &operator*() const { return descriptorPool; }
	vk::DescriptorPool		 &operator*() { return descriptorPool; }
};

class DescriptorSetLayout {
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::Device				device;

   public:
	void clear() {
		if (descriptorSetLayout) {
			dbLog(dbg::LOG_INFO, "Destroying descriptor set layout ", (void *)descriptorSetLayout);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			descriptorSetLayout = nullptr;
		}
	}

	DescriptorSetLayout(std::nullptr_t) : descriptorSetLayout(nullptr), device(nullptr) {}
	DescriptorSetLayout(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout)
		: descriptorSetLayout(descriptorSetLayout), device(device) {}
	~DescriptorSetLayout() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(DescriptorSetLayout);
	DescriptorSetLayout(DescriptorSetLayout &&other) noexcept
		: descriptorSetLayout(other.descriptorSetLayout), device(other.device) {
		other.descriptorSetLayout = nullptr;
	}
	DescriptorSetLayout &operator=(DescriptorSetLayout &&other) noexcept {
		if (this != &other) {
			clear();
			descriptorSetLayout		  = other.descriptorSetLayout;
			device					  = other.device;
			other.descriptorSetLayout = nullptr;
		}
		return *this;
	}

								   operator vk::DescriptorSetLayout() const { return descriptorSetLayout; }
								   operator VkDescriptorSetLayout() const { return descriptorSetLayout; }
	const vk::DescriptorSetLayout &operator*() const { return descriptorSetLayout; }
	vk::DescriptorSetLayout		  &operator*() { return descriptorSetLayout; }
};

class DescriptorSet {
	vk::DescriptorSet  descriptorSet;
	vk::Device		   device;
	vk::DescriptorPool descriptorPool;

   public:
	void clear() {
		if (descriptorSet) {
			dbLog(dbg::LOG_WARNING, "Descriptor deletion is not implemented, see " __FILE__ " : " STR(__LINE__));
			// TODO:
			// Currently the resources in the descriptor set are not tracked and some of them
			// may die before the descriptor set itself, so we can't free the descriptor set without risking
			// use-after-free errors.

			// vkFreeDescriptorSets(device, (VkDescriptorPool)descriptorPool, 1, (VkDescriptorSet *)&*descriptorSet);
			descriptorSet = nullptr;
		}
	}

	DescriptorSet(std::nullptr_t) : descriptorSet(nullptr), device(nullptr), descriptorPool(nullptr) {}
	DescriptorSet(vk::Device device, vk::DescriptorPool descriptorPool, vk::DescriptorSet descriptorSet)
		: descriptorSet(descriptorSet), device(device), descriptorPool(descriptorPool) {}
	~DescriptorSet() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(DescriptorSet);
	DescriptorSet(DescriptorSet &&other) noexcept
		: descriptorSet(other.descriptorSet), device(other.device), descriptorPool(other.descriptorPool) {
		other.descriptorSet = nullptr;
	}
	DescriptorSet &operator=(DescriptorSet &&other) noexcept {
		if (this != &other) {
			clear();
			descriptorSet		= other.descriptorSet;
			device				= other.device;
			descriptorPool		= other.descriptorPool;
			other.descriptorSet = nullptr;
		}
		return *this;
	}

							 operator vk::DescriptorSet() const { return descriptorSet; }
							 operator VkDescriptorSet() const { return descriptorSet; }
	const vk::DescriptorSet &operator*() const { return descriptorSet; }
	vk::DescriptorSet		&operator*() { return descriptorSet; }
};

class DeviceMemory {
	vk::DeviceMemory memory;
	vk::Device		 device;

   public:
	void clear() {
		if (memory) {
			vkFreeMemory(device, memory, nullptr);
			memory = nullptr;
		}
	}

	DeviceMemory(std::nullptr_t) : memory(nullptr), device(nullptr) {}
	DeviceMemory(vk::Device device, vk::DeviceMemory memory) : memory(memory), device(device) {}
	~DeviceMemory() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(DeviceMemory);
	DeviceMemory(DeviceMemory &&other) noexcept : memory(other.memory), device(other.device) { other.memory = nullptr; }
	DeviceMemory &operator=(DeviceMemory &&other) noexcept {
		if (this != &other) {
			clear();
			memory		 = other.memory;
			device		 = other.device;
			other.memory = nullptr;
		}
		return *this;
	}

							operator vk::DeviceMemory() const { return memory; }
							operator VkDeviceMemory() const { return memory; }
	const vk::DeviceMemory &operator*() const { return memory; }
	vk::DeviceMemory	   &operator*() { return memory; }
};

class Sampler {
	vk::Sampler sampler;
	vk::Device	device;

   public:
	void clear() {
		if (sampler) {
			vkDestroySampler(device, sampler, nullptr);
			sampler = nullptr;
		}
	}

	Sampler(std::nullptr_t) : sampler(nullptr), device(nullptr) {}
	Sampler(vk::Device device, vk::Sampler sampler) : sampler(sampler), device(device) {}
	~Sampler() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Sampler);
	Sampler(Sampler &&other) noexcept : sampler(other.sampler), device(other.device) { other.sampler = nullptr; }
	Sampler &operator=(Sampler &&other) noexcept {
		if (this != &other) {
			clear();
			sampler		  = other.sampler;
			device		  = other.device;
			other.sampler = nullptr;
		}
		return *this;
	}

					   operator vk::Sampler() const { return sampler; }
					   operator VkSampler() const { return sampler; }
	const vk::Sampler &operator*() const { return sampler; }
	vk::Sampler		  &operator*() { return sampler; }
};

class CommandPool {
	vk::CommandPool commandPool;
	vk::Device		device;

   public:
	void clear() {
		if (commandPool) {
			vkDestroyCommandPool(device, commandPool, nullptr);
			commandPool = nullptr;
		}
	}

	CommandPool(std::nullptr_t) : commandPool(nullptr), device(nullptr) {}
	CommandPool(vk::Device device, vk::CommandPool commandPool) : commandPool(commandPool), device(device) {}
	~CommandPool() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(CommandPool);
	CommandPool(CommandPool &&other) noexcept : commandPool(other.commandPool), device(other.device) {
		other.commandPool = nullptr;
	}
	CommandPool &operator=(CommandPool &&other) noexcept {
		if (this != &other) {
			clear();
			commandPool		  = other.commandPool;
			device			  = other.device;
			other.commandPool = nullptr;
		}
		return *this;
	}

						   operator vk::CommandPool() const { return commandPool; }
						   operator VkCommandPool() const { return commandPool; }
	const vk::CommandPool &operator*() const { return commandPool; }
	vk::CommandPool		  &operator*() { return commandPool; }
};

class Queue {
	vk::Queue  queue;
	vk::Device device;

   public:
	void clear() {
		// Queues are implicitly freed when the device is destroyed, so we don't need to do anything here
		queue = nullptr;
	}

	Queue(std::nullptr_t) : queue(nullptr), device(nullptr) {}
	Queue(vk::Device device, vk::Queue queue) : queue(queue), device(device) {}
	~Queue() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Queue);
	Queue(Queue &&other) noexcept : queue(other.queue), device(other.device) { other.queue = nullptr; }
	Queue &operator=(Queue &&other) noexcept {
		if (this != &other) {
			clear();
			queue		= other.queue;
			device		= other.device;
			other.queue = nullptr;
		}
		return *this;
	}

					 operator vk::Queue() const { return queue; }
					 operator VkQueue() const { return queue; }
	const vk::Queue &operator*() const { return queue; }
	vk::Queue		&operator*() { return queue; }
};

class CommandBuffer {
	vk::CommandBuffer commandBuffer;
	vk::Device		  device;

   public:
	void clear() {
		if (commandBuffer) {
			// Command buffers are implicitly freed when the command pool is destroyed, so we don't need to do anything
			// here
			commandBuffer = nullptr;
		}
	}

	CommandBuffer(std::nullptr_t) : commandBuffer(nullptr), device(nullptr) {}
	CommandBuffer(vk::Device device, vk::CommandBuffer commandBuffer) : commandBuffer(commandBuffer), device(device) {}
	~CommandBuffer() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(CommandBuffer);
	CommandBuffer(CommandBuffer &&other) noexcept : commandBuffer(other.commandBuffer), device(other.device) {
		other.commandBuffer = nullptr;
	}
	CommandBuffer &operator=(CommandBuffer &&other) noexcept {
		if (this != &other) {
			clear();
			commandBuffer		= other.commandBuffer;
			device				= other.device;
			other.commandBuffer = nullptr;
		}
		return *this;
	}

							 operator vk::CommandBuffer() const { return commandBuffer; }
							 operator VkCommandBuffer() const { return commandBuffer; }
	const vk::CommandBuffer &operator*() const { return commandBuffer; }
	vk::CommandBuffer		&operator*() { return commandBuffer; }
};

class ShaderModule {
	vk::ShaderModule shaderModule;
	vk::Device		 device;

   public:
	void clear() {
		if (shaderModule) {
			vkDestroyShaderModule(device, shaderModule, nullptr);
			shaderModule = nullptr;
		}
	}

	ShaderModule(std::nullptr_t) : shaderModule(nullptr), device(nullptr) {}
	ShaderModule(vk::Device device, vk::ShaderModule shaderModule) : shaderModule(shaderModule), device(device) {}
	~ShaderModule() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(ShaderModule);
	ShaderModule(ShaderModule &&other) noexcept : shaderModule(other.shaderModule), device(other.device) {
		other.shaderModule = nullptr;
	}
	ShaderModule &operator=(ShaderModule &&other) noexcept {
		if (this != &other) {
			clear();
			shaderModule	   = other.shaderModule;
			device			   = other.device;
			other.shaderModule = nullptr;
		}
		return *this;
	}

							operator vk::ShaderModule() const { return shaderModule; }
							operator VkShaderModule() const { return shaderModule; }
	const vk::ShaderModule &operator*() const { return shaderModule; }
	vk::ShaderModule	   &operator*() { return shaderModule; }
};

class Pipeline {
	vk::Pipeline pipeline;
	vk::Device	 device;

   public:
	void clear() {
		if (pipeline) {
			vkDestroyPipeline(device, pipeline, nullptr);
			pipeline = nullptr;
		}
	}

	Pipeline(std::nullptr_t) : pipeline(nullptr), device(nullptr) {}
	Pipeline(vk::Device device, vk::Pipeline pipeline) : pipeline(pipeline), device(device) {}
	~Pipeline() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Pipeline);
	Pipeline(Pipeline &&other) noexcept : pipeline(other.pipeline), device(other.device) { other.pipeline = nullptr; }
	Pipeline &operator=(Pipeline &&other) noexcept {
		if (this != &other) {
			clear();
			pipeline	   = other.pipeline;
			device		   = other.device;
			other.pipeline = nullptr;
		}
		return *this;
	}

						operator vk::Pipeline() const { return pipeline; }
						operator VkPipeline() const { return pipeline; }
	const vk::Pipeline &operator*() const { return pipeline; }
	vk::Pipeline	   &operator*() { return pipeline; }
};

class PipelineLayout {
	vk::PipelineLayout pipelineLayout;
	vk::Device		   device;

   public:
	void clear() {
		if (pipelineLayout) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			pipelineLayout = nullptr;
		}
	}

	PipelineLayout(std::nullptr_t) : pipelineLayout(nullptr), device(nullptr) {}
	PipelineLayout(vk::Device device, vk::PipelineLayout pipelineLayout)
		: pipelineLayout(pipelineLayout), device(device) {}
	~PipelineLayout() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(PipelineLayout);
	PipelineLayout(PipelineLayout &&other) noexcept : pipelineLayout(other.pipelineLayout), device(other.device) {
		other.pipelineLayout = nullptr;
	}
	PipelineLayout &operator=(PipelineLayout &&other) noexcept {
		if (this != &other) {
			clear();
			pipelineLayout		 = other.pipelineLayout;
			device				 = other.device;
			other.pipelineLayout = nullptr;
		}
		return *this;
	}

							  operator vk::PipelineLayout() const { return pipelineLayout; }
							  operator VkPipelineLayout() const { return pipelineLayout; }
	const vk::PipelineLayout &operator*() const { return pipelineLayout; }
	vk::PipelineLayout		 &operator*() { return pipelineLayout; }
};

class Semaphore {
	vk::Semaphore semaphore;
	vk::Device	  device;

   public:
	void clear() {
		if (semaphore) {
			vkDestroySemaphore(device, semaphore, nullptr);
			semaphore = nullptr;
		}
	}

	Semaphore(std::nullptr_t) : semaphore(nullptr), device(nullptr) {}
	Semaphore(vk::Device device, vk::Semaphore semaphore) : semaphore(semaphore), device(device) {}
	~Semaphore() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Semaphore);
	Semaphore(Semaphore &&other) noexcept : semaphore(other.semaphore), device(other.device) {
		other.semaphore = nullptr;
	}
	Semaphore &operator=(Semaphore &&other) noexcept {
		if (this != &other) {
			clear();
			semaphore		= other.semaphore;
			device			= other.device;
			other.semaphore = nullptr;
		}
		return *this;
	}

						 operator vk::Semaphore() const { return semaphore; }
						 operator VkSemaphore() const { return semaphore; }
	const vk::Semaphore &operator*() const { return semaphore; }
	vk::Semaphore		&operator*() { return semaphore; }
};

class Fence {
	vk::Fence  fence;
	vk::Device device;

   public:
	void clear() {
		if (fence) {
			vkDestroyFence(device, fence, nullptr);
			fence = nullptr;
		}
	}

	Fence(std::nullptr_t) : fence(nullptr), device(nullptr) {}
	Fence(vk::Device device, vk::Fence fence) : fence(fence), device(device) {}
	~Fence() { clear(); }
	DELETE_COPY_AND_ASSIGNMENT(Fence);
	Fence(Fence &&other) noexcept : fence(other.fence), device(other.device) { other.fence = nullptr; }
	Fence &operator=(Fence &&other) noexcept {
		if (this != &other) {
			clear();
			fence		= other.fence;
			device		= other.device;
			other.fence = nullptr;
		}
		return *this;
	}

					 operator vk::Fence() const { return fence; }
					 operator VkFence() const { return fence; }
	const vk::Fence &operator*() const { return fence; }
	vk::Fence		&operator*() { return fence; }
};

};	   // namespace vkraii
