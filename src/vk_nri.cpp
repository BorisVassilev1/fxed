#include "vk_nri.hpp"
#include <iostream>
#include <ostream>
#include <fstream>
#include <format>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
	#include <wrl.h>
	#include <dxcapi.h>
	#pragma comment(lib, "dxcompiler.lib")
#else
	#include <dxc/dxcapi.h>
#endif

#ifdef NDEBUG
	#include "shader_registry.hpp"
#endif

#include "dxc_include_handler.hpp"
#include "nriFactory.hpp"
#include "nri.hpp"

namespace nri {
static const std::vector<const char *> validationLayers = {
#ifndef NDEBUG
	"VK_LAYER_KHRONOS_validation"
#endif
};

#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = true;
#endif

static VkFormat nriFormat2vkFormat[] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R4G4_UNORM_PACK8,
	VK_FORMAT_R4G4B4A4_UNORM_PACK16,
	VK_FORMAT_B4G4R4A4_UNORM_PACK16,
	VK_FORMAT_R5G6B5_UNORM_PACK16,
	VK_FORMAT_B5G6R5_UNORM_PACK16,
	VK_FORMAT_R5G5B5A1_UNORM_PACK16,
	VK_FORMAT_B5G5R5A1_UNORM_PACK16,
	VK_FORMAT_A1R5G5B5_UNORM_PACK16,
	VK_FORMAT_R8_UNORM,
	VK_FORMAT_R8_SNORM,
	VK_FORMAT_R8_USCALED,
	VK_FORMAT_R8_SSCALED,
	VK_FORMAT_R8_UINT,
	VK_FORMAT_R8_SINT,
	VK_FORMAT_R8_SRGB,
	VK_FORMAT_R8G8_UNORM,
	VK_FORMAT_R8G8_SNORM,
	VK_FORMAT_R8G8_USCALED,
	VK_FORMAT_R8G8_SSCALED,
	VK_FORMAT_R8G8_UINT,
	VK_FORMAT_R8G8_SINT,
	VK_FORMAT_R8G8_SRGB,
	VK_FORMAT_R8G8B8_UNORM,
	VK_FORMAT_R8G8B8_SNORM,
	VK_FORMAT_R8G8B8_USCALED,
	VK_FORMAT_R8G8B8_SSCALED,
	VK_FORMAT_R8G8B8_UINT,
	VK_FORMAT_R8G8B8_SINT,
	VK_FORMAT_R8G8B8_SRGB,
	VK_FORMAT_B8G8R8_UNORM,
	VK_FORMAT_B8G8R8_SNORM,
	VK_FORMAT_B8G8R8_USCALED,
	VK_FORMAT_B8G8R8_SSCALED,
	VK_FORMAT_B8G8R8_UINT,
	VK_FORMAT_B8G8R8_SINT,
	VK_FORMAT_B8G8R8_SRGB,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SNORM,
	VK_FORMAT_R8G8B8A8_USCALED,
	VK_FORMAT_R8G8B8A8_SSCALED,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SNORM,
	VK_FORMAT_B8G8R8A8_USCALED,
	VK_FORMAT_B8G8R8A8_SSCALED,
	VK_FORMAT_B8G8R8A8_UINT,
	VK_FORMAT_B8G8R8A8_SINT,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_A8B8G8R8_UNORM_PACK32,
	VK_FORMAT_A8B8G8R8_SNORM_PACK32,
	VK_FORMAT_A8B8G8R8_USCALED_PACK32,
	VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
	VK_FORMAT_A8B8G8R8_UINT_PACK32,
	VK_FORMAT_A8B8G8R8_SINT_PACK32,
	VK_FORMAT_A8B8G8R8_SRGB_PACK32,
	VK_FORMAT_A2R10G10B10_UNORM_PACK32,
	VK_FORMAT_A2R10G10B10_SNORM_PACK32,
	VK_FORMAT_A2R10G10B10_USCALED_PACK32,
	VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
	VK_FORMAT_A2R10G10B10_UINT_PACK32,
	VK_FORMAT_A2R10G10B10_SINT_PACK32,
	VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	VK_FORMAT_A2B10G10R10_SNORM_PACK32,
	VK_FORMAT_A2B10G10R10_USCALED_PACK32,
	VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
	VK_FORMAT_A2B10G10R10_UINT_PACK32,
	VK_FORMAT_A2B10G10R10_SINT_PACK32,
	VK_FORMAT_R16_UNORM,
	VK_FORMAT_R16_SNORM,
	VK_FORMAT_R16_USCALED,
	VK_FORMAT_R16_SSCALED,
	VK_FORMAT_R16_UINT,
	VK_FORMAT_R16_SINT,
	VK_FORMAT_R16_SFLOAT,
	VK_FORMAT_R16G16_UNORM,
	VK_FORMAT_R16G16_SNORM,
	VK_FORMAT_R16G16_USCALED,
	VK_FORMAT_R16G16_SSCALED,
	VK_FORMAT_R16G16_UINT,
	VK_FORMAT_R16G16_SINT,
	VK_FORMAT_R16G16_SFLOAT,
	VK_FORMAT_R16G16B16_UNORM,
	VK_FORMAT_R16G16B16_SNORM,
	VK_FORMAT_R16G16B16_USCALED,
	VK_FORMAT_R16G16B16_SSCALED,
	VK_FORMAT_R16G16B16_UINT,
	VK_FORMAT_R16G16B16_SINT,
	VK_FORMAT_R16G16B16_SFLOAT,
	VK_FORMAT_R16G16B16A16_UNORM,
	VK_FORMAT_R16G16B16A16_SNORM,
	VK_FORMAT_R16G16B16A16_USCALED,
	VK_FORMAT_R16G16B16A16_SSCALED,
	VK_FORMAT_R16G16B16A16_UINT,
	VK_FORMAT_R16G16B16A16_SINT,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R32_UINT,
	VK_FORMAT_R32_SINT,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_UINT,
	VK_FORMAT_R32G32_SINT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_UINT,
	VK_FORMAT_R32G32B32_SINT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_UINT,
	VK_FORMAT_R32G32B32A32_SINT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_R64_UINT,
	VK_FORMAT_R64_SINT,
	VK_FORMAT_R64_SFLOAT,
	VK_FORMAT_R64G64_UINT,
	VK_FORMAT_R64G64_SINT,
	VK_FORMAT_R64G64_SFLOAT,
	VK_FORMAT_R64G64B64_UINT,
	VK_FORMAT_R64G64B64_SINT,
	VK_FORMAT_R64G64B64_SFLOAT,
	VK_FORMAT_R64G64B64A64_UINT,
	VK_FORMAT_R64G64B64A64_SINT,
	VK_FORMAT_R64G64B64A64_SFLOAT,
	VK_FORMAT_B10G11R11_UFLOAT_PACK32,
	VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
	VK_FORMAT_D16_UNORM,
	VK_FORMAT_X8_D24_UNORM_PACK32,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_S8_UINT,
	VK_FORMAT_D16_UNORM_S8_UINT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_D32_SFLOAT_S8_UINT,
};

bool checkValidationLayerSupport() {
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char *layerName : validationLayers) {
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) { return false; }
	}

	return true;
}

std::pair<const char *, bool> extensions[] = {
	{"VK_KHR_surface", true},
#ifdef __linux__
	{"VK_KHR_xlib_surface", true}, {"VK_KHR_wayland_surface", false}, {"VK_KHR_xcb_surface", false},
#elif defined(_WIN32)
	{"VK_KHR_win32_surface", true},
#endif
	{"VK_EXT_debug_utils", false},
};

static std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
													 VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
													 VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME};

std::vector<const char *> checkExtensionSupport() {
	auto					  availableExtensions = vk::enumerateInstanceExtensionProperties();
	std::vector<const char *> availableExtensionNames;

	for (const auto &[extensionName, required] : extensions) {
		bool extensionFound = false;

		for (const auto &extensionProperties : availableExtensions) {
			if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound) {
			if (required) throw std::runtime_error(std::format("Required extension not found! : {}", extensionName));
			else dbLog(dbg::LOG_WARNING, "Optional extension not found: ", extensionName);
		} else {
			availableExtensionNames.push_back(extensionName);
		}
	}
	return availableExtensionNames;
}

static VulkanNRI::QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice &device) {
	VulkanNRI::QueueFamilyIndices indices;

	auto queueFamilies = device.getQueueFamilyProperties();
	dbLog(dbg::LOG_DEBUG, "Found ", queueFamilies.size(), " queue families.");

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		// supports graphics, compute, and transfer
		if (queueFamily.queueFlags &
			vk::QueueFlagBits::eGraphics	 // &&
											 // queueFamily.queueFlags & vk::QueueFlagBits::eCompute &&
											 // queueFamily.queueFlags & vk::QueueFlagBits::eTransfer
		) {
			indices.graphicsFamily = i;
			dbLog(dbg::LOG_INFO, "Queue family ", i, " supports graphics, compute, and transfer.");
		}
		i++;
	}

	if (indices.graphicsFamily.has_value()) {
		dbLog(dbg::LOG_INFO, "Found graphics queue family index: ", indices.graphicsFamily.value());
	} else {
		dbLog(dbg::LOG_WARNING, "No suitable graphics queue family found.");
	}

	return indices;
}

static bool isDeviceSuitable(const vk::raii::PhysicalDevice &device) {
	VulkanNRI::QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = true;
	auto supportedExtensions = device.enumerateDeviceExtensionProperties();
	for (const auto &requiredExt : deviceExtensions) {
		auto it = std::find_if(
			supportedExtensions.begin(), supportedExtensions.end(),
			[&](const vk::ExtensionProperties &ext) { return strcmp(ext.extensionName, requiredExt) == 0; });
		if (it == supportedExtensions.end()) {
			dbLog(dbg::LOG_WARNING, "Required device extension not supported: ", requiredExt);
			extensionsSupported = false;
		}
	}
	if (!extensionsSupported) return false;

	if (!indices.graphicsFamily.has_value()) return false;

	return true;
}

static void printDeviceQueueFamiliesInfo(const vk::raii::PhysicalDevice &device) {
	auto queueFamilies = device.getQueueFamilyProperties();
	dbLog(dbg::LOG_DEBUG, "Device has ", queueFamilies.size(), " queue families:");
	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		dbLog(dbg::LOG_DEBUG, " Queue Family ", i, ":");
		dbLog(dbg::LOG_DEBUG, "  Queue Count: ", queueFamily.queueCount);
		dbLog(dbg::LOG_DEBUG, "  Queue Flags: ");
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) { dbLog(dbg::LOG_DEBUG, "   - Graphics"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) { dbLog(dbg::LOG_DEBUG, "   - Compute"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) { dbLog(dbg::LOG_DEBUG, "   - Transfer"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eSparseBinding) {
			dbLog(dbg::LOG_DEBUG, "   - Sparse Binding");
		}
		if (queueFamily.queueFlags & vk::QueueFlagBits::eProtected) { dbLog(dbg::LOG_DEBUG, "   - Protected"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR) { dbLog(dbg::LOG_DEBUG, "   - Video Decode"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR) { dbLog(dbg::LOG_DEBUG, "   - Video Encode"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eOpticalFlowNV) { dbLog(dbg::LOG_DEBUG, "   - Optical Flow"); }
		i++;
	}
}

void VulkanNRI::createInstance() {
	vk::ApplicationInfo appInfo("VulkanApp", 1, "NoEngine", 1, vk::ApiVersion12);

	vk::InstanceCreateInfo createInfo({}, &appInfo);

	if (enableValidationLayers) {
		if (!checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	auto availableExtensions = checkExtensionSupport();

	createInfo.enabledExtensionCount   = availableExtensions.size();
	createInfo.ppEnabledExtensionNames = availableExtensions.data();

	instance = vk::raii::Instance(vk::raii::Context(), createInfo);
}

void VulkanNRI::pickPhysicalDevice() {
	auto physicalDevices = instance.enumeratePhysicalDevices();
	if (physicalDevices.size() == 0) { throw std::runtime_error("Failed to find GPUs with Vulkan support!"); }

	for (const auto &device : physicalDevices) {
		dbLog(dbg::LOG_INFO, "Found device: ", device.getProperties().deviceName);
		// printDeviceQueueFamiliesInfo(device);
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	queueFamilyIndices = findQueueFamilies(physicalDevice);
	if (physicalDevice == nullptr) { throw std::runtime_error("Failed to find a suitable GPU!"); }
}

void VulkanNRI::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	float					  prio = 1.0f;
	vk::DeviceQueueCreateInfo queueCreateInfo({}, indices.graphicsFamily.value(), 1, &prio);

	// check device features support

	vk::PhysicalDeviceFeatures					 deviceFeatures{};
	vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures(
		VK_FALSE,	  // shaderInputAttachmentArrayDynamicIndexing
		VK_TRUE,	  // shaderUniformTexelBufferArrayDynamicIndexing
		VK_TRUE,	  // shaderStorageTexelBufferArrayDynamicIndexing
		VK_TRUE,	  // shaderUniformBufferArrayNonUniformIndexing
		VK_TRUE,	  // shaderStorageBufferArrayNonUniformIndexing
		VK_TRUE,	  // shaderSampledImageArrayNonUniformIndexing
		VK_TRUE,	  // shaderStorageImageArrayNonUniformIndexing
		VK_FALSE,	  // shaderInputAttachmentArrayNonUniformIndexing
		VK_TRUE,	  // shaderUniformTexelBufferArrayNonUniformIndexing
		VK_TRUE,	  // shaderStorageTexelBufferArrayNonUniformIndexing
		VK_TRUE,	  // descriptorBindingUniformBufferUpdateAfterBind
		VK_TRUE,	  // descriptorBindingStorageBufferUpdateAfterBind
		VK_TRUE,	  // descriptorBindingSampledImageUpdateAfterBind
		VK_TRUE,	  // descriptorBindingStorageImageUpdateAfterBind
		VK_TRUE,	  // descriptorBindingStorageTexelBufferUpdateAfterBind
		VK_TRUE,	  // descriptorBindingUniformTexelBufferUpdateAfterBind
		VK_TRUE,	  // descriptorBindingUpdateUnusedWhilePending
		VK_TRUE,	  // descriptorBindingPartiallyBound
		VK_TRUE,	  // descriptorBindingVariableDescriptorCount
		VK_TRUE,	  // runtimeDescriptorArray
		nullptr);
	vk::PhysicalDeviceDynamicRenderingFeatures	  dynamicRenderingFeature(VK_TRUE, &descriptorIndexingFeatures);
	vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeature(VK_TRUE, VK_FALSE, VK_FALSE,
																			 &dynamicRenderingFeature);

	vk::DeviceCreateInfo createInfo(
		{}, 1, &queueCreateInfo, enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
		enableValidationLayers ? validationLayers.data() : nullptr, static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.data(), &deviceFeatures, &bufferDeviceAddressFeature);

	device = vk::raii::Device(physicalDevice, createInfo);
}
}	  // namespace nri
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
namespace nri {
static void prepareGLFW() {
	if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW"); }
	if (!glfwVulkanSupported()) { throw std::runtime_error("GLFW: Vulkan not supported"); }

	uint32_t	 count		= 0;
	const char **extensions = glfwGetRequiredInstanceExtensions(&count);
	if (extensions == nullptr) { throw std::runtime_error("GLFW: Failed to get required instance extensions"); }

	dbLog(dbg::LOG_INFO, "GLFW required instance extensions:");
	for (uint32_t i = 0; i < count; i++) {
		dbLog(dbg::LOG_INFO, " ", extensions[i]);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

VulkanNRI::VulkanNRI(CreateBits createBits)
	: NRI(createBits),
	  instance(nullptr),
	  physicalDevice(nullptr),
	  device(nullptr),
	  defaultCommandPool(nullptr),
	  descriptorAllocator(std::nullopt) {
	if (createBits & CreateBits::GLFW) { prepareGLFW(); }

	createInstance();
	pickPhysicalDevice();
	createLogicalDevice();

	auto dcp		   = createCommandPool();
	defaultCommandPool = std::move(static_cast<VulkanCommandPool &>(*dcp));
	descriptorAllocator.emplace(*this);

	dbLog(dbg::LOG_INFO, "VulkanNRI initialized with device: ", physicalDevice.getProperties().deviceName);
}

VulkanNRI::~VulkanNRI() {
	device.waitIdle();
	// VulkanMemoryCache::destroy();
}
VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanNRI &nri)
	: nri(nri), pool(nullptr), descriptorSetLayout(nullptr), bigDescriptorSet(nullptr) {
	std::array<vk::DescriptorPoolSize, 4> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 500),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 500),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 500),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 500),
	};

	vk::DescriptorPoolCreateInfo poolInfo(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind, 1000,
		poolSizes.size(), poolSizes.data());

	pool = vk::raii::DescriptorPool(nri.getDevice(), poolInfo);

	std::array<vk::DescriptorSetLayoutBinding, 4> samplerBindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 500, vk::ShaderStageFlagBits::eAll,
									   nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 500, vk::ShaderStageFlagBits::eAll,
									   nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, 500, vk::ShaderStageFlagBits::eAll,
									   nullptr),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 500, vk::ShaderStageFlagBits::eAll,
									   nullptr),
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool |
													 vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT,
												 samplerBindings.size(), samplerBindings.data());

	const vk::DescriptorBindingFlags bindingFlags = vk::DescriptorBindingFlagBits::eUpdateAfterBind;

	const vk::DescriptorBindingFlags bindingFlagsArr[] = {bindingFlags, bindingFlags, bindingFlags, bindingFlags,
														  bindingFlags};

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo(samplerBindings.size(), bindingFlagsArr);
	layoutInfo.pNext = &bindingFlagsInfo;

	descriptorSetLayout = vk::raii::DescriptorSetLayout(nri.getDevice(), layoutInfo);
	vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &*descriptorSetLayout);
	auto						  descriptorSets = nri.getDevice().allocateDescriptorSets(allocInfo);
	bigDescriptorSet							 = std::move(descriptorSets[0]);
}

ResourceHandle VulkanDescriptorAllocator::addUniformBufferDescriptor(const VulkanBuffer &buffer) {
	uint32_t descriptorIndex = currentBufferDescriptorIndex++;

	vk::DescriptorBufferInfo bufferInfo(buffer.getBuffer(), 0, VK_WHOLE_SIZE);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 2, descriptorIndex, 1, vk::DescriptorType::eUniformBuffer,
										   nullptr, &bufferInfo, nullptr);

	(*nri.getDevice()).updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_BUFFER, false, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addStorageBufferDescriptor(const VulkanBuffer &buffer) {
	uint32_t descriptorIndex = currentBufferDescriptorIndex++;

	dbLog(dbg::LOG_INFO, "Adding storage buffer descriptor at index ", descriptorIndex);
	vk::DescriptorBufferInfo bufferInfo(buffer.getBuffer(), 0, VK_WHOLE_SIZE);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 3, descriptorIndex, 1, vk::DescriptorType::eStorageBuffer,
										   nullptr, &bufferInfo, nullptr);

	(*nri.getDevice()).updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_BUFFER, true, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addSamplerImageDescriptor(const VulkanTexture2D &image) {
	uint32_t descriptorIndex = currentImageDescriptorIndex++;

	vk::DescriptorImageInfo imageInfo(*(image.getSampler()), *(image.get()), vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 0, descriptorIndex, 1,
										   vk::DescriptorType::eCombinedImageSampler, &imageInfo, nullptr, nullptr);

	(*nri.getDevice()).updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_IMAGE_SAMPLER, false, descriptorIndex);
}
ResourceHandle VulkanDescriptorAllocator::addAccelerationStructureDescriptor(const VulkanTLAS &tlas) {
	uint32_t descriptorIndex = currentASDescriptorIndex++;

	vk::WriteDescriptorSetAccelerationStructureKHR descriptorASInfo(1, &*const_cast<VulkanTLAS &>(tlas).getTLAS());
	vk::WriteDescriptorSet						   descriptorWrite(bigDescriptorSet, 4, descriptorIndex, 1,
																   vk::DescriptorType::eAccelerationStructureKHR, nullptr, nullptr, nullptr,
																   &descriptorASInfo);
	(*nri.getDevice()).updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_TLAS, false, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addStorageImageDescriptor(const VulkanStorageImage2D &image) {
	uint32_t descriptorIndex = currentImageDescriptorIndex++;

	vk::DescriptorImageInfo imageInfo(nullptr, *(image.get()), vk::ImageLayout::eGeneral);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 1, descriptorIndex, 1, vk::DescriptorType::eStorageImage,
										   &imageInfo, nullptr, nullptr);

	(*nri.getDevice()).updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_STORAGE_IMAGE, true, descriptorIndex);
}

vk::MemoryPropertyFlags typeRequest2vkMemoryProperty[] = {
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
	vk::MemoryPropertyFlagBits::eDeviceLocal};

VulkanAllocation::VulkanAllocation(VulkanNRI &nri, MemoryRequirements memoryRequirements)
	: memory(nullptr), device(nri.getDevice()), size(memoryRequirements.size) {
	assert(memoryRequirements.typeRequest >= 0);
	assert(memoryRequirements.typeRequest < MemoryTypeRequest::_MEMORY_TYPE_NUM);
	vk::MemoryPropertyFlags properties = typeRequest2vkMemoryProperty[(uint32_t)memoryRequirements.typeRequest];

	vk::PhysicalDeviceMemoryProperties memProperties   = nri.getPhysicalDevice().getMemoryProperties();
	uint32_t						   memoryTypeIndex = -1;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == -1u) { throw std::runtime_error("Failed to find suitable memory type!"); }

	vk::MemoryAllocateFlagsInfo allocFlagsInfo(vk::MemoryAllocateFlagBits::eDeviceAddress, {});
	vk::MemoryAllocateInfo		allocInfo(memoryRequirements.size, memoryTypeIndex, &allocFlagsInfo);

	memory = vk::raii::DeviceMemory(nri.getDevice(), allocInfo);
}

ResourceHandle VulkanBuffer::createHandle() const {
	return nri->getDescriptorAllocator().addStorageBufferDescriptor(*this);
}

VulkanBuffer::VulkanBuffer(VulkanNRI &nri, std::size_t size, BufferUsage usage)
	: nri(&nri), buffer(nullptr), allocation(nullptr), offset(0), size(size) {
	vk::BufferUsageFlags bufferUsageFlags;
	if (usage & BufferUsage::BUFFER_USAGE_VERTEX)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_INDEX)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_UNIFORM) bufferUsageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
	if (usage & BufferUsage::BUFFER_USAGE_STORAGE)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_TRANSFER_SRC) bufferUsageFlags |= vk::BufferUsageFlagBits::eTransferSrc;
	if (usage & BufferUsage::BUFFER_USAGE_TRANSFER_DST) bufferUsageFlags |= vk::BufferUsageFlagBits::eTransferDst;
	if (usage & BufferUsage::BUFFER_USAGE_ACCELERATION_STRUCTURE)
		bufferUsageFlags |=
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_SHADER_BINDING_TABLE)
		bufferUsageFlags |=
			vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;

	vk::BufferCreateInfo bufferInfo({}, size, bufferUsageFlags, vk::SharingMode::eExclusive);

	this->buffer = vk::raii::Buffer(nri.getDevice(), bufferInfo);
}

MemoryRequirements VulkanBuffer::getMemoryRequirements() {
	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

	return MemoryRequirements(memRequirements.size, MemoryTypeRequest::MEMORY_TYPE_DEVICE, memRequirements.alignment);
}

void VulkanBuffer::bindMemory(Allocation &allocation, std::size_t offset) {
	VulkanAllocation &vulkanAllocation = static_cast<VulkanAllocation &>(allocation);
	this->offset					   = offset;

	buffer.bindMemory(vulkanAllocation.getMemory(), offset);
	this->allocation = &vulkanAllocation;
}
void *VulkanBuffer::map(std::size_t offset, std::size_t size) {
	assert(allocation != nullptr);
	void *data = nullptr;
	vkMapMemory(allocation->getDevice(), allocation->getMemory(), offset, size, 0, &data);
	if (data == nullptr) { throw std::runtime_error("Failed to map buffer memory."); }
	return data;
}

void VulkanBuffer::unmap() {
	assert(allocation != nullptr);

	// vk::MappedMemoryRange memoryRange(allocation->getMemory(), offset, VK_WHOLE_SIZE);
	// auto				  res = allocation->getDevice().flushMappedMemoryRanges(1, &memoryRange);
	// assert(res == vk::Result::eSuccess);

	vkUnmapMemory(allocation->getDevice(), allocation->getMemory());
}

std::size_t VulkanBuffer::getSize() const { return this->size; }

std::size_t VulkanBuffer::getOffset() const { return this->offset; }

void VulkanBuffer::copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
							std::size_t dstOffset, std::size_t size) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	auto &vkSrcBuf = static_cast<VulkanBuffer &>(srcBuffer);

	if (!vkCmdBuf.isRecording) vkCmdBuf.begin();

	// TODO: put the right barriers here
	// vk::BufferMemoryBarrier bufferBarrier(vk::AccessFlagBits::eHostWrite,
	//									  vk::AccessFlagBits::eTransferRead, VK_QUEUE_FAMILY_IGNORED,
	//									  VK_QUEUE_FAMILY_IGNORED, vkSrcBuf.buffer, srcOffset, size);
	// vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost,
	//									   vk::PipelineStageFlagBits::eTransfer, {}, {}, {bufferBarrier}, {});

	vkCmdBuf.commandBuffer.copyBuffer(vkSrcBuf.buffer, this->buffer, vk::BufferCopy(srcOffset, dstOffset, size));

	vk::BufferMemoryBarrier bufferBarrier2(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
										   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, this->buffer, dstOffset,
										   size);
	vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe,
										   {}, {}, {bufferBarrier2}, {});
}

void VulkanBuffer::bindAsVertexBuffer(CommandBuffer &commandBuffer, uint32_t binding, std::size_t offset,
									  std::size_t stride) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	static_cast<void>(stride);

	vk::Buffer vkBuffer = this->buffer;

	vkCmdBuf.commandBuffer.bindVertexBuffers(binding, vkBuffer, vk::DeviceSize(offset));
}

vk::IndexType nriIndexType2vkIndexType[] = {
	vk::IndexType::eUint16,
	vk::IndexType::eUint32,
};

void VulkanBuffer::bindAsIndexBuffer(CommandBuffer &commandBuffer, std::size_t offset, IndexType indexType) {
	auto		 &vkCmdBuf	  = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vk::Buffer	  vkBuffer	  = this->buffer;
	vk::IndexType vkIndexType = nriIndexType2vkIndexType[static_cast<int>(indexType)];
	assert(int(vkIndexType) != -1);
	vkCmdBuf.commandBuffer.bindIndexBuffer(vkBuffer, vk::DeviceSize(offset), vkIndexType);
}

vk::DeviceAddress VulkanBuffer::getAddress() {
	vk::BufferDeviceAddressInfo addressInfo;
	addressInfo.setBuffer(buffer);
	return nri->getDevice().getBufferAddress(addressInfo);
}

VulkanImageView::VulkanImageView(VulkanNRI &nri, vk::raii::ImageView &&imgView, vk::Format fmt)
	: ImageView(), nri(&nri), imageView(std::move(imgView)), format(fmt) {}

VulkanRenderTarget::VulkanRenderTarget(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	imageView = vk::raii::ImageView(nri.getDevice(), imageViewInfo);
}

ResourceHandle VulkanRenderTarget::createHandle() const {
	dbLog(dbg::LOG_ERROR, "VulkanRenderTarget::createHandle not implemented!");
	return ResourceHandle::INVALID_HANDLE;
}

VulkanTexture2D::VulkanTexture2D(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()), sampler(nullptr) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	imageView = vk::raii::ImageView(nri.getDevice(), imageViewInfo);

	vk::SamplerCreateInfo samplerInfo({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
									  vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
									  vk::SamplerAddressMode::eClampToEdge, 0.0f, VK_FALSE, 1.0f, VK_FALSE,
									  vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack,
									  VK_FALSE);
	sampler = vk::raii::Sampler(nri.getDevice(), samplerInfo);
}

ResourceHandle VulkanTexture2D::createHandle() const {
	return nri->getDescriptorAllocator().addSamplerImageDescriptor(*this);
}

VulkanStorageImage2D::VulkanStorageImage2D(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	imageView = vk::raii::ImageView(nri.getDevice(), imageViewInfo);
}

ResourceHandle VulkanStorageImage2D::createHandle() const {
	return nri->getDescriptorAllocator().addStorageImageDescriptor(*this);
}

MemoryRequirements &MemoryRequirements::setTypeRequest(MemoryTypeRequest tr) {
	typeRequest = tr;
	return *this;
}

MemoryRequirements &MemoryRequirements::setAlignment(std::size_t a) {
	alignment = a;
	return *this;
}

std::unique_ptr<Image2D> VulkanNRI::createImage2D(uint32_t width, uint32_t height, Format format, ImageUsage usage) {
	return std::make_unique<VulkanImage2D>(*this, width, height, format, usage);
}

std::unique_ptr<Buffer> VulkanNRI::createBuffer(std::size_t size, BufferUsage usage) {
	return std::make_unique<VulkanBuffer>(*this, size, usage);
}

MemoryRequirements VulkanImage2D::getMemoryRequirements() {
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(**device, image.get(), &memRequirements);

	return MemoryRequirements(memRequirements.size, MemoryTypeRequest::MEMORY_TYPE_DEVICE, memRequirements.alignment);
}

void VulkanImage2D::bindMemory(Allocation &allocation, std::size_t offset) {
	VulkanAllocation &vulkanAllocation = static_cast<VulkanAllocation &>(allocation);

	vkBindImageMemory(**device, image.get(), vulkanAllocation.getMemory(), offset);
}

void VulkanImage2D::clear(CommandBuffer &commandBuffer, glm::vec4 color) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	vk::ClearColorValue v{color.r, color.g, color.b, color.a};
	vkBuf.begin();

	transitionLayout(vkBuf, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eNone,
					 vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
					 vk::PipelineStageFlagBits::eTransfer);

	vkBuf.commandBuffer.clearColorImage(image.get(), vk::ImageLayout::eTransferDstOptimal, v,
										{vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)});
}

void VulkanImage2D::prepareForPresent(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::ePresentSrcKHR,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eMemoryRead,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eBottomOfPipe);
}
void VulkanImage2D::prepareForStorage(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::eGeneral,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader |
						 vk::PipelineStageFlagBits::eVertexShader);
}

void VulkanImage2D::prepareForTexture(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::eShaderReadOnlyOptimal,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eShaderRead,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eVertexShader |
						 vk::PipelineStageFlagBits::eComputeShader);
}

void VulkanImage2D::copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
							 uint32_t srcRowPitch) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	auto &vkSrcBuf = static_cast<VulkanBuffer &>(srcBuffer);
	if (!vkCmdBuf.isRecording) vkCmdBuf.begin();
	transitionLayout(vkCmdBuf, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eNone,
					 vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
					 vk::PipelineStageFlagBits::eTransfer);
	vk::BufferImageCopy region(srcOffset, srcRowPitch, 0, vk::ImageSubresourceLayers(aspectFlags, 0, 0, 1),
							   vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));
	vkCmdBuf.commandBuffer.copyBufferToImage(vkSrcBuf.getBuffer(), image.get(), vk::ImageLayout::eTransferDstOptimal,
											 region);
}

vk::ImageAspectFlags VulkanImage2D::getAspectFlags(vk::Format format) {
	switch (format) {
		case vk::Format::eUndefined: throw std::runtime_error("Undefined format has no aspect flags.");
		default: break;
		case vk::Format::eD32SfloatS8Uint:
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD16UnormS8Uint: return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil; ;
		case vk::Format::eD32Sfloat:
		case vk::Format::eD16Unorm: return vk::ImageAspectFlagBits::eDepth;
	}
	return vk::ImageAspectFlagBits::eColor;
}

void VulkanImage2D::transitionLayout(CommandBuffer &commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcAccess,
									 vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage,
									 vk::PipelineStageFlags dstStage) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	vk::ImageMemoryBarrier barrier(srcAccess, dstAccess, layout, newLayout, vk::QueueFamilyIgnored,
								   vk::QueueFamilyIgnored, image.get(), {aspectFlags, 0, 1, 0, 1});
	vkBuf.begin();
	vkBuf.commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits::eByRegion, {}, {}, {barrier});
}

VulkanImage2D::VulkanImage2D(VulkanNRI &nri, uint32_t width, uint32_t height, Format format, ImageUsage usage)
	: nri(&nri),
	  image(nullptr),
	  device(&nri.getDevice()),
	  layout(vk::ImageLayout::eUndefined),
	  format((vk::Format)nriFormat2vkFormat[static_cast<int>(format)]),
	  aspectFlags(getAspectFlags(this->format)),
	  width(width),
	  height(height) {
	assert(format != Format::FORMAT_UNDEFINED);
	assert(format < Format::_FORMAT_NUM);
	assert(this->format != vk::Format::eUndefined);
	assert(int(this->format) != -1);
	assert(width > 0 && height > 0);
	vk::Format vkFormat = (vk::Format)nriFormat2vkFormat[static_cast<int>(format)];

	vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, vk::Format(vkFormat),
								  vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1), 1, 1,
								  vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
								  static_cast<vk::ImageUsageFlagBits>(usage), vk::SharingMode::eExclusive, 0, nullptr,
								  vk::ImageLayout::eUndefined);

	this->image = vk::raii::Image(nri.getDevice(), imageInfo);
}

std::unique_ptr<ImageView> VulkanImage2D::createRenderTargetView() {
	return std::make_unique<VulkanRenderTarget>(*nri, *this);
}

std::unique_ptr<ImageView> VulkanImage2D::createTextureView() { return std::make_unique<VulkanTexture2D>(*nri, *this); }

std::unique_ptr<ImageView> VulkanImage2D::createStorageView() {
	return std::make_unique<VulkanStorageImage2D>(*nri, *this);
}

std::unique_ptr<CommandPool> VulkanNRI::createCommandPool() {
	vk::CommandPoolCreateInfo poolCI(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
									 queueFamilyIndices.graphicsFamily.value());

	vk::raii::CommandPool pool = vk::raii::CommandPool(device, poolCI);

	return std::make_unique<VulkanCommandPool>(std::move(pool));
}

std::unique_ptr<ProgramBuilder> VulkanNRI::createProgramBuilder() {
	return std::make_unique<VulkanProgramBuilder>(*this);
}

std::unique_ptr<CommandQueue> VulkanNRI::createCommandQueue() {
	vk::raii::Queue queue = vk::raii::Queue(device, queueFamilyIndices.graphicsFamily.value(), 0);

	return std::make_unique<VulkanCommandQueue>(std::move(queue));
};

VulkanWindow::VulkanWindow(VulkanNRI &nri)
	: Window(nri),
	  surface(nullptr),
	  swapChain(nullptr),
	  presentQueue(nullptr),
	  imageAvailableSemaphore(nri.getDevice(), vk::SemaphoreCreateInfo()),
	  renderFinishedSemaphore(nri.getDevice(), vk::SemaphoreCreateInfo()),
	  inFlightFence(nri.getDevice(), vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
	  depthImage(std::nullopt) {
	if (imageAvailableSemaphore == nullptr || renderFinishedSemaphore == nullptr || inFlightFence == nullptr) {
		throw std::runtime_error("Failed to create synchronization objects for a frame!");
	}
	commandBuffer = std::unique_ptr<VulkanCommandBuffer>(
		(VulkanCommandBuffer *)nri.createCommandBuffer(nri.getDefaultCommandPool()).release());
}

static std::pair<vk::Format, vk::ColorSpaceKHR> chooseSurfaceFormat(
	const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
	// if the surface format list only includes one entry with eUndefined,
	// there is no preferred format, so we assume eB8G8R8A8Unorm
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
		return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
	}

	// check for preferred format
	for (const auto &availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return {availableFormat.format, availableFormat.colorSpace};
		}
	}

	// otherwise just pick the first available
	return {availableFormats[0].format, availableFormats[0].colorSpace};
}

void VulkanWindow::setSurface(vk::raii::SurfaceKHR &&surface) {
	auto &nri = static_cast<VulkanNRI &>(this->nri);

	this->surface = std::move(surface);

	auto surfaceformats = nri.getPhysicalDevice().getSurfaceFormatsKHR(*this->surface);
	if (surfaceformats.empty()) { throw std::runtime_error("Failed to get Vulkan surface formats!"); }
	std::tie(surfaceFormat, surfaceColorSpace) = chooseSurfaceFormat(surfaceformats);
	dbLog(dbg::LOG_INFO, "Chosen surface format: ", vk::to_string(surfaceFormat),
		  ", color space: ", vk::to_string(surfaceColorSpace));
}

void VulkanWindow::createSwapChain(uint32_t &width, uint32_t &height) {
	this->width	 = width;
	this->height = height;
	auto &nri	 = static_cast<VulkanNRI &>(this->nri);
	if (this->surface == nullptr) { THROW_RUNTIME_ERR("surface is not set for VulkanWindow!"); }

	vk::SurfaceCapabilitiesKHR capabilities = nri.getPhysicalDevice().getSurfaceCapabilitiesKHR(*surface);
	width									= capabilities.currentExtent.width;
	width									= std::min(capabilities.maxImageExtent.width, width);
	height									= capabilities.currentExtent.height;
	height									= std::min(capabilities.maxImageExtent.height, height);

	dbLog(dbg::LOG_INFO, "Creating swap chain with size: ", width, "x", height);

	vk::SwapchainCreateInfoKHR swapChainInfo(
		{}, *surface, capabilities.minImageCount + 1, this->surfaceFormat, this->surfaceColorSpace, {width, height}, 1,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, 0,
		nullptr, vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo, VK_TRUE, *swapChain);

	static PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR =
		reinterpret_cast<PFN_vkCreateSwapchainKHR>(nri.getInstance().getProcAddr("vkCreateSwapchainKHR"));
	assert(vkCreateSwapchainKHR != nullptr);

	VkSwapchainKHR _swapChain;
	if (vkCreateSwapchainKHR(*nri.getDevice(), (VkSwapchainCreateInfoKHR *)&swapChainInfo, nullptr, &_swapChain) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan swap chain!");
	}

	this->swapChainImages.clear();
	vk::SwapchainKHR oldswapchain = swapChain.release();
	vkDestroySwapchainKHR(*nri.getDevice(), oldswapchain, nullptr);
	swapChain			 = vk::raii::SwapchainKHR(nri.getDevice(), _swapChain);
	auto swapChainImages = swapChain.getImages();
	for (const auto &image : swapChainImages) {
		VulkanImage2D nriImage = VulkanImage2D(nri, image, vk::ImageLayout::eUndefined, vk::Format::eB8G8R8A8Unorm,
											   nri.getDevice(), width, height);

		VulkanRenderTarget renderTarget = VulkanRenderTarget(nri, nriImage);

		this->swapChainImages.emplace_back(std::move(nriImage), std::move(renderTarget));
		this->swapChainImages.back().image.transitionLayout(
			*commandBuffer, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eMemoryRead,
			vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe);
	}

	depthImage =
		VulkanImage2D(nri, width, height, Format::FORMAT_D32_SFLOAT, ImageUsage::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT);
	depthImageAllocation = VulkanAllocation(
		nri, depthImage->getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	depthImage->bindMemory(*depthImageAllocation, 0);

	depthImage->transitionLayout(
		*commandBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);
	depthImageView = VulkanRenderTarget(nri, *depthImage);
}

void VulkanWindow::beginFrame() {
	auto &nri = static_cast<const VulkanNRI &>(this->nri);

	auto result = nri.getDevice().waitForFences({inFlightFence}, VK_TRUE, UINT64_MAX);
	assert(result == vk::Result::eSuccess);
	nri.getDevice().resetFences({inFlightFence});
	assert(result == vk::Result::eSuccess);
	auto imageIndex			= swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphore, nullptr);
	this->currentImageIndex = imageIndex.value;

	swapChainImages[imageIndex.value].image.transitionLayout(
		*commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eMemoryRead,
		vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

ImageAndViewRef VulkanWindow::getCurrentRenderTarget() {
	auto &sci = swapChainImages[currentImageIndex];
	return ImageAndViewRef(sci.image, sci.view);
}
CommandBuffer &VulkanWindow::getCurrentCommandBuffer() { return *commandBuffer; }

void VulkanWindow::endFrame() {
	getCurrentRenderTarget().image.prepareForPresent(*commandBuffer);

	commandBuffer->end();
	auto				  &nri	  = static_cast<const VulkanNRI &>(this->nri);
	vk::PipelineStageFlags stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	presentQueue.queue.submit(vk::SubmitInfo(1, &(*imageAvailableSemaphore), &stages, 1, &*commandBuffer->commandBuffer,
											 1, &(*renderFinishedSemaphore)),
							  *inFlightFence);

	vk::PresentInfoKHR presentInfo =
		vk::PresentInfoKHR(1, &*renderFinishedSemaphore, 1, &*swapChain, &currentImageIndex, nullptr);

	// result = presentQueue.presentKHR(presentInfo);
	vk::Result res = (vk::Result)vkQueuePresentKHR(*presentQueue.queue, &*presentInfo);
	switch (res) {
		case vk::Result::eSuccess: break;
		case vk::Result::eErrorOutOfDateKHR:
		case vk::Result::eSuboptimalKHR:
			vkDeviceWaitIdle(*nri.getDevice());
			uint32_t width, height;
			createSwapChain(width, height);
			break;
		case vk::Result::eErrorSurfaceLostKHR: break;
		default: assert(res == vk::Result::eSuccess);
	}

	presentQueue.queue.waitIdle();
}

void VulkanWindow::beginRendering(CommandBuffer &cmdBuf, const ImageAndViewRef &renderTarget) {
	auto *rtp = dynamic_cast<const VulkanRenderTarget *>(&renderTarget.view);
	assert(rtp != nullptr);
	auto &rt	   = *rtp;
	auto &img	   = static_cast<const VulkanImage2D &>(renderTarget.image);
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(cmdBuf);

	vk::RenderingInfo renderingInfo{};
	renderingInfo.sType		 = vk::StructureType::eRenderingInfo;
	renderingInfo.renderArea = vk::Rect2D({0, 0}, {img.getWidth(), img.getHeight()});
	vk::RenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType		= vk::StructureType::eRenderingAttachmentInfo;
	colorAttachment.imageView	= rt.get();
	colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.loadOp		= vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp		= vk::AttachmentStoreOp::eStore;
	vk::ClearValue clearValue;
	clearValue.color.setFloat32({this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a});
	colorAttachment.clearValue		   = clearValue;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments	   = &colorAttachment;
	renderingInfo.layerCount		   = 1;

	vk::RenderingAttachmentInfo depthAttachment{};
	depthAttachment.sType		= vk::StructureType::eRenderingAttachmentInfo;
	depthAttachment.imageView	= depthImageView->get();
	depthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttachment.loadOp		= vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp		= vk::AttachmentStoreOp::eDontCare;
	vk::ClearValue depthClearValue;
	depthClearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
	depthAttachment.clearValue	 = depthClearValue;

	renderingInfo.pDepthAttachment = &depthAttachment;

	vkCmdBuf.commandBuffer.beginRendering(renderingInfo);

	vkCmdBuf.commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(img.getWidth()),
													   static_cast<float>(img.getHeight()), 0.0f, 1.0f));
	vkCmdBuf.commandBuffer.setScissor(0, vk::Rect2D({0, 0}, {img.getWidth(), img.getHeight()}));
}

void VulkanWindow::endRendering(CommandBuffer &cmdBuf) {
	static_cast<VulkanCommandBuffer &>(cmdBuf).commandBuffer.endRendering();
}

vk::PrimitiveTopology nriPrimitiveType2vkTopology[] = {
	vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleStrip, vk::PrimitiveTopology::eLineList,
	vk::PrimitiveTopology::eLineStrip,	  vk::PrimitiveTopology::ePointList,
};

// TODO: merge with DX12 implementation
std::pair<std::vector<vk::raii::ShaderModule>, std::vector<vk::PipelineShaderStageCreateInfo>> VulkanProgramBuilder::
	createShaderModules(std::vector<ShaderCreateInfo> &&stagesInfo, const vk::raii::Device &device) {
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vk::raii::ShaderModule>			   shaderModules;

	CComPtr<IDxcCompiler3> compiler;
	HRESULT				   hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	assert(SUCCEEDED(hr) && "Failed to create DX Compiler.");

	CComPtr<IDxcUtils> utils;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	assert(SUCCEEDED(hr) && "Failed to create DX Utils.");

	auto includeHandler = std::make_unique<CustomIncludeHandler>(utils);

	for (const auto &stageInfo : stagesInfo) {
		vk::ShaderStageFlagBits stage;
		switch (stageInfo.shaderType) {
			case ShaderType::SHADER_TYPE_VERTEX: stage = vk::ShaderStageFlagBits::eVertex; break;
			case ShaderType::SHADER_TYPE_FRAGMENT: stage = vk::ShaderStageFlagBits::eFragment; break;
			case ShaderType::SHADER_TYPE_COMPUTE: stage = vk::ShaderStageFlagBits::eCompute; break;
			case ShaderType::SHADER_TYPE_RAYGEN: stage = vk::ShaderStageFlagBits::eRaygenKHR; break;
			case ShaderType::SHADER_TYPE_CLOSEST_HIT: stage = vk::ShaderStageFlagBits::eClosestHitKHR; break;
			case ShaderType::SHADER_TYPE_ANY_HIT: stage = vk::ShaderStageFlagBits::eAnyHitKHR; break;
			case ShaderType::SHADER_TYPE_MISS: stage = vk::ShaderStageFlagBits::eMissKHR; break;
			default:
				dbLog(dbg::LOG_ERROR, "Unsupported shader type: ", static_cast<int>(stageInfo.shaderType));
				throw std::runtime_error("Unsupported shader stage!");
		}
		const std::filesystem::path sourceFile{stageInfo.sourceFile};

		std::string filename = sourceFile.filename().string();
		std::replace(filename.begin(), filename.end(), '.', '_');
		std::string cacheFileName = std::format("{}_{}.spv", filename, stageInfo.entryPoint);
#ifndef NDEBUG
		CComPtr<IDxcBlob> sourceBlob;
		std::wstring	  wSourceFile = std::wstring(stageInfo.sourceFile.begin(), stageInfo.sourceFile.end());
		HRESULT			  hr		  = includeHandler->LoadSource(wSourceFile.c_str(), &sourceBlob);

		if (FAILED(hr)) {
			dbLog(dbg::LOG_ERROR, "Failed to load shader source file: ", stageInfo.sourceFile);
			THROW_RUNTIME_ERR(std::format("Failed to load shader source file: {}", stageInfo.sourceFile));
		}

		std::vector<LPCWSTR> arguments;
		arguments.push_back(L"-E");
		std::wstring entryPoint = std::wstring(stageInfo.entryPoint.begin(), stageInfo.entryPoint.end());
		arguments.push_back(entryPoint.c_str());
		arguments.push_back(L"-T");
		switch (stageInfo.shaderType) {
			case ShaderType::SHADER_TYPE_VERTEX: arguments.push_back(L"vs_6_6"); break;
			case ShaderType::SHADER_TYPE_FRAGMENT: arguments.push_back(L"ps_6_6"); break;
			case ShaderType::SHADER_TYPE_COMPUTE: arguments.push_back(L"cs_6_6"); break;
			case ShaderType::SHADER_TYPE_RAYGEN: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_CLOSEST_HIT: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_ANY_HIT: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_MISS: arguments.push_back(L"lib_6_6"); break;
			default:
				dbLog(dbg::LOG_ERROR, "Unsupported shader type: ", static_cast<int>(stageInfo.shaderType));
				throw std::runtime_error("Unsupported shader stage!");
		}
		arguments.push_back(L"-spirv");
		arguments.push_back(L"-D");
		arguments.push_back(L"VULKAN");
		arguments.push_back(L"-D");
		arguments.push_back(L"SHADER");
		arguments.push_back(L"-I");
		arguments.push_back(L"./shaders/");
		arguments.push_back(L"-fvk-use-dx-layout");
		arguments.push_back(L"-fspv-target-env=vulkan1.2");
		arguments.push_back(L"-HV");
		arguments.push_back(L"2021");

		DxcBuffer buffer{};
		buffer.Ptr		= sourceBlob->GetBufferPointer();
		buffer.Size		= sourceBlob->GetBufferSize();
		buffer.Encoding = 0;

		dbLog(dbg::LOG_DEBUG, "\n\tCompiling shader: ", stageInfo.sourceFile,
			  "\n\tEntry point: ", stageInfo.entryPoint);

		includeHandler->reset();
		CComPtr<IDxcResult> result;
		hr = compiler->Compile(&buffer, arguments.data(), static_cast<UINT32>(arguments.size()), includeHandler.get(),
							   IID_PPV_ARGS(&result));
		if (FAILED(hr)) { throw std::runtime_error(std::format("Failed to compile shader: {}", stageInfo.sourceFile)); }

		CComPtr<IDxcBlobEncoding> errorBuffer;
		result->GetErrorBuffer(&errorBuffer);
		if (errorBuffer != nullptr && errorBuffer->GetBufferSize() > 0) {
			std::string errorMessage(reinterpret_cast<const char *>(errorBuffer->GetBufferPointer()),
									 errorBuffer->GetBufferSize());
			std::cerr << "Shader compilation warnings/errors: " << errorMessage << std::endl;
		}

		CComPtr<IDxcBlob> spirvBlob;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirvBlob), nullptr);

		// write to shader cache
		std::filesystem::path cachePath = std::filesystem::path("shadercache") / cacheFileName;
		std::filesystem::create_directories(cachePath.parent_path());
		std::ofstream ofs(cachePath, std::ios::binary);
		ofs.write(reinterpret_cast<const char *>(spirvBlob->GetBufferPointer()), spirvBlob->GetBufferSize());
		ofs.close();
		vk::ShaderModuleCreateInfo shaderModuleInfo({}, spirvBlob->GetBufferSize(),
													reinterpret_cast<const uint32_t *>(spirvBlob->GetBufferPointer()));
#else	  // NDEBUG
		auto it = g_shaders.find(cacheFileName);
		if (it == g_shaders.end()) {
			dbLog(dbg::LOG_ERROR, "Shader cache not found for: ", cacheFileName);
			THROW_RUNTIME_ERR(std::format("Shader cache not found for: {}", cacheFileName));
		}
		auto					  &shaderdata = it->second;
		vk::ShaderModuleCreateInfo shaderModuleInfo({}, shaderdata.length,
													reinterpret_cast<const uint32_t *>(shaderdata.data));
#endif

		shaderModules.emplace_back(device, shaderModuleInfo);

		vk::PipelineShaderStageCreateInfo shaderStageInfo({}, stage, *shaderModules.back(),
														  stageInfo.entryPoint.c_str());
		shaderStages.push_back(shaderStageInfo);
	}
	return {std::move(shaderModules), std::move(shaderStages)};
}

std::vector<vk::PushConstantRange> VulkanProgramBuilder::createPushConstantRanges(
	const std::vector<PushConstantRange> &nriPushConstantRanges) {
	std::vector<vk::PushConstantRange> vkPushConstantRanges;
	for (const auto &nriPushConstantRange : nriPushConstantRanges) {
		vkPushConstantRanges.emplace_back(vk::ShaderStageFlagBits::eAll, nriPushConstantRange.offset,
										  nriPushConstantRange.size);
	}
	return vkPushConstantRanges;
}

vk::VertexInputRate nriInputRate2vkInputRate[] = {
	vk::VertexInputRate::eVertex,
	vk::VertexInputRate::eInstance,
};

std::unique_ptr<GraphicsProgram> VulkanProgramBuilder::buildGraphicsProgram() {
	auto [shaderModules, shaderStages] = createShaderModules(std::move(shaderStagesInfo), nri.getDevice());

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	auto						 pushConstantRanges = createPushConstantRanges(this->pushConstantRanges);
	pipelineLayoutInfo.setPushConstantRangeCount(pushConstantRanges.size());
	pipelineLayoutInfo.setPPushConstantRanges(pushConstantRanges.data());
	pipelineLayoutInfo.setSetLayoutCount(1);
	pipelineLayoutInfo.setPSetLayouts(&*nri.getDescriptorAllocator().getDescriptorSetLayout());
	auto pipelineLayout = vk::raii::PipelineLayout(nri.getDevice(), pipelineLayoutInfo);

	std::vector<vk::VertexInputBindingDescription>	 bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	for (const auto &binding : this->vertexBindings) {
		vk::VertexInputRate inputRate = nriInputRate2vkInputRate[static_cast<int>(binding.inputRate)];
		assert(int(inputRate) != -1);
		vk::VertexInputBindingDescription bindingDescription(binding.binding, binding.stride, inputRate);
		bindingDescriptions.push_back(bindingDescription);

		for (const auto &attribute : binding.attributes) {
			vk::VertexInputAttributeDescription attributeDescription(attribute.location, binding.binding,
																	 vk::Format(attribute.format), attribute.offset);
			attributeDescriptions.push_back(attributeDescription);
		}
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, bindingDescriptions.size(), bindingDescriptions.data(),
														   attributeDescriptions.size(), attributeDescriptions.data());

	vk::PrimitiveTopology vkTopology = nriPrimitiveType2vkTopology[static_cast<int>(primitiveType)];
	assert(int(vkTopology) != -1);
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, vkTopology, VK_FALSE);

	vk::Viewport						viewport(0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f);
	vk::Rect2D							scissor({0, 0}, {800, 600});
	vk::PipelineViewportStateCreateInfo viewportStateInfo({}, 1, &viewport, 1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
															vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
															VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisampleInfo({}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr,
														   VK_FALSE, VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment,
														{0.0f, 0.0f, 0.0f, 0.0f});

	std::array<vk::DynamicState, 3>	   dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
														vk::DynamicState::eBlendConstants};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates.size(), dynamicStates.data());

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo({}, VK_FALSE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE,
															 VK_FALSE, vk::StencilOpState(), vk::StencilOpState(), 0.0f,
															 1.0f);

	vk::Format						colorFormat = vk::Format::eB8G8R8A8Unorm;
	vk::PipelineRenderingCreateInfo pipelineRenderingInfo(0, 1, &colorFormat, vk::Format::eD32Sfloat);

	vk::GraphicsPipelineCreateInfo pipelineInfo(
		{}, static_cast<uint32_t>(shaderStages.size()), shaderStages.data(), &vertexInputInfo, &inputAssemblyInfo,
		nullptr,	 // tessellation
		&viewportStateInfo, &rasterizerInfo, &multisampleInfo, &depthStencilInfo, &colorBlending, &dynamicStateInfo,
		*pipelineLayout, vk::RenderPass(nullptr), 0, nullptr, -1, &pipelineRenderingInfo);

	auto pipelines = nri.getDevice().createGraphicsPipelines(nullptr, {pipelineInfo});
	return std::make_unique<VulkanGraphicsProgram>(nri, std::move(pipelines[0]), std::move(pipelineLayout));
}

std::unique_ptr<ComputeProgram> VulkanProgramBuilder::buildComputeProgram() {
	auto [shaderModules, shaderStages] = createShaderModules(std::move(shaderStagesInfo), nri.getDevice());
	assert(shaderStages.size() == 1 && "Compute program must have exactly one shader stage.");

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	auto						 pipelineLayout = vk::raii::PipelineLayout(nri.getDevice(), pipelineLayoutInfo);

	vk::ComputePipelineCreateInfo pipelineInfo({}, shaderStages[0], *pipelineLayout);

	auto pipelines = nri.getDevice().createComputePipelines(nullptr, {pipelineInfo});
	return std::make_unique<VulkanComputeProgram>(nri, std::move(pipelines[0]), std::move(pipelineLayout));
}

static auto findShaderStage(const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
							vk::ShaderStageFlagBits								  stage) {
	return std::find_if(shaderStages.begin(), shaderStages.end(),
						[stage](const vk::PipelineShaderStageCreateInfo &s) { return s.stage == stage; });
}

std::unique_ptr<RayTracingProgram> VulkanProgramBuilder::buildRayTracingProgram(nri::CommandBuffer &commandBuffer) {
	auto [shaderModules, shaderStages] = createShaderModules(std::move(shaderStagesInfo), nri.getDevice());

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	auto						 pushConstantRanges = createPushConstantRanges(this->pushConstantRanges);
	pipelineLayoutInfo.setPushConstantRangeCount(pushConstantRanges.size());
	pipelineLayoutInfo.setPPushConstantRanges(pushConstantRanges.data());
	pipelineLayoutInfo.setSetLayoutCount(1);
	pipelineLayoutInfo.setPSetLayouts(&*nri.getDescriptorAllocator().getDescriptorSetLayout());
	auto pipelineLayout = vk::raii::PipelineLayout(nri.getDevice(), pipelineLayoutInfo);

	// find ray generation shader stage
	auto raygenit	  = findShaderStage(shaderStages, vk::ShaderStageFlagBits::eRaygenKHR);
	auto closesthitit = findShaderStage(shaderStages, vk::ShaderStageFlagBits::eClosestHitKHR);
	auto anyhitit	  = findShaderStage(shaderStages, vk::ShaderStageFlagBits::eAnyHitKHR);
	auto missit		  = findShaderStage(shaderStages, vk::ShaderStageFlagBits::eMissKHR);

	auto raygen		= raygenit != shaderStages.end() ? raygenit - shaderStages.begin() : VK_SHADER_UNUSED_KHR;
	auto closesthit = closesthitit != shaderStages.end() ? closesthitit - shaderStages.begin() : VK_SHADER_UNUSED_KHR;
	auto anyhit		= anyhitit != shaderStages.end() ? anyhitit - shaderStages.begin() : VK_SHADER_UNUSED_KHR;
	auto miss		= missit != shaderStages.end() ? missit - shaderStages.begin() : VK_SHADER_UNUSED_KHR;

	if (raygen == VK_SHADER_UNUSED_KHR) dbLog(dbg::LOG_ERROR, "No ray generation shader found in ray tracing program!");
	// if (miss != VK_SHADER_UNUSED_KHR) dbLog(dbg::LOG_ERROR, "Miss shaders not currently supported in VulkanNRI");

	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
	if (raygen != VK_SHADER_UNUSED_KHR)
		shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral,
																	  raygen, VK_SHADER_UNUSED_KHR,
																	  VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR));
	if (miss != VK_SHADER_UNUSED_KHR)
		shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, miss,
																	  VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
																	  VK_SHADER_UNUSED_KHR));
	if (closesthit != VK_SHADER_UNUSED_KHR)
		shaderGroups.push_back(
			vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
												   VK_SHADER_UNUSED_KHR, closesthit, anyhit, VK_SHADER_UNUSED_KHR));

	vk::RayTracingPipelineCreateInfoKHR pipelineInfo({}, static_cast<uint32_t>(shaderStages.size()),
													 shaderStages.data(), shaderGroups.size(), shaderGroups.data(),
													 1,		// no recursion for now
													 {}, {}, {}, *pipelineLayout, {}, {});

	static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR =
		reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
			nri.getInstance().getProcAddr("vkCreateRayTracingPipelinesKHR"));
	assert(vkCreateRayTracingPipelinesKHR != nullptr);

	auto pipelines = nri.getDevice().createRayTracingPipelinesKHR(nullptr, nullptr, pipelineInfo, nullptr);
	if (!pipelines.size()) dbLog(dbg::LOG_ERROR, "Failed to create ray tracing pipeline!");

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
	rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 properties2{};
	properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	properties2.pNext = &rtProps;
	vkGetPhysicalDeviceProperties2(*nri.getPhysicalDevice(), &properties2);

	// Calculate aligned sizes
	uint32_t handleSize		   = rtProps.shaderGroupHandleSize;
	uint32_t handleAlignment   = rtProps.shaderGroupHandleAlignment;
	uint32_t baseAlignment	   = rtProps.shaderGroupBaseAlignment;
	uint32_t alignedHandleSize = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);
	alignedHandleSize		   = (alignedHandleSize + baseAlignment - 1) & ~(baseAlignment - 1);

	uint32_t numGroups = static_cast<uint32_t>(shaderGroups.size());
	uint32_t sbtSize   = numGroups * alignedHandleSize;
	dbLog(dbg::LOG_DEBUG, "SBT size: ", sbtSize, " bytes (", numGroups, " groups of ", alignedHandleSize,
		  " bytes each).");

	std::vector<uint8_t>							handleData(numGroups * handleSize);
	static PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR =
		reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
			nri.getInstance().getProcAddr("vkGetRayTracingShaderGroupHandlesKHR"));

	vkGetRayTracingShaderGroupHandlesKHR(*nri.getDevice(), *pipelines[0], 0, numGroups, handleData.size(),
										 handleData.data());

	VulkanBuffer sbtBuffer =
		VulkanBuffer(nri, sbtSize,
					 BufferUsage::BUFFER_USAGE_SHADER_BINDING_TABLE | BufferUsage::BUFFER_USAGE_TRANSFER_SRC |
						 BufferUsage::BUFFER_USAGE_TRANSFER_DST);
	VulkanAllocation sbtAllocation = VulkanAllocation(nri, sbtBuffer.getMemoryRequirements()
															   .setAlignment(baseAlignment)
															   .setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	sbtBuffer.bindMemory(sbtAllocation, 0);

	VulkanBuffer	 uploadBuffer	  = VulkanBuffer(nri, sbtSize, BufferUsage::BUFFER_USAGE_TRANSFER_SRC);
	VulkanAllocation uploadAllocation = VulkanAllocation(
		nri, uploadBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_UPLOAD));
	uploadBuffer.bindMemory(uploadAllocation, 0);

	{
		void		  *mappedData = uploadBuffer.map(0, sbtSize);
		uint8_t		  *dst		  = reinterpret_cast<uint8_t *>(mappedData);
		const uint8_t *src		  = handleData.data();
		for (uint32_t i = 0; i < numGroups; i++) {
			std::memcpy(dst, src, handleSize);
			dst += alignedHandleSize;
			src += handleSize;
		}
		uploadBuffer.unmap();
	}
	sbtBuffer.copyFrom(commandBuffer, uploadBuffer, 0, 0, sbtSize);

	auto result = std::make_unique<VulkanRayTracingProgram>(nri, std::move(pipelines[0]), std::move(pipelineLayout),
															std::move(sbtBuffer), std::move(sbtAllocation),
															std::move(uploadBuffer), std::move(uploadAllocation));

	dbLog(dbg::LOG_DEBUG, "sbtBuffer: ", *result->sbtBuffer.getBuffer());
	dbLog(dbg::LOG_DEBUG, "sbtBuffer: address = ", std::hex, result->sbtBuffer.getAddress(), std::dec);

	result->sbtRayGenRegion.deviceAddress = result->sbtBuffer.getAddress();
	result->sbtRayGenRegion.stride		  = alignedHandleSize;
	result->sbtRayGenRegion.size		  = alignedHandleSize;
	result->sbtMissRegion.deviceAddress =
		result->sbtBuffer.getAddress() + (raygen != VK_SHADER_UNUSED_KHR ? alignedHandleSize : 0);
	result->sbtMissRegion.stride	   = alignedHandleSize;
	result->sbtMissRegion.size		   = (miss != VK_SHADER_UNUSED_KHR ? alignedHandleSize : 0);
	result->sbtHitRegion.deviceAddress = result->sbtBuffer.getAddress() +
										 (raygen != VK_SHADER_UNUSED_KHR ? alignedHandleSize : 0) +
										 (miss != VK_SHADER_UNUSED_KHR ? alignedHandleSize : 0);
	result->sbtHitRegion.stride = alignedHandleSize;
	result->sbtHitRegion.size	= (closesthit != VK_SHADER_UNUSED_KHR ? alignedHandleSize : 0);
	return result;
}

void VulkanRayTracingProgram::traceRays(CommandBuffer &commandBuffer, uint32_t width, uint32_t height, uint32_t depth) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	vkCmdBuf.begin();
	vkCmdBuf.commandBuffer.traceRaysKHR(this->sbtRayGenRegion, this->sbtMissRegion, this->sbtHitRegion, {}, width,
										height, depth);
};

void VulkanProgram::bind(CommandBuffer &commandBuffer) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdBuf.commandBuffer.bindPipeline(bindPoint, *pipeline);

	auto &descriptorSet = static_cast<VulkanNRI &>(nri).getDescriptorAllocator().getDescriptorSet();
	vkCmdBuf.commandBuffer.bindDescriptorSets(bindPoint, *pipelineLayout, 0, *descriptorSet, {});
}

void VulkanProgram::unbind(CommandBuffer &commandBuffer) {
	static_cast<void>(commandBuffer);
	// No unbind in Vulkan
}

void VulkanProgram::setPushConstants(CommandBuffer &commandBuffer, const void *data, std::size_t size,
									 std::size_t offset) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdPushConstants(*vkCmdBuf.commandBuffer, *pipelineLayout, (VkShaderStageFlags)vk::ShaderStageFlagBits::eAll,
					   offset, size, data);
}

void VulkanGraphicsProgram::draw(CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
								 uint32_t firstVertex, uint32_t firstInstance) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdBuf.commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanGraphicsProgram::drawIndexed(CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount,
										uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdBuf.commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanComputeProgram::dispatch(CommandBuffer &commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
									uint32_t groupCountZ) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdBuf.commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
}

void VulkanMemoryCache::assureSize(VulkanNRI &nri, std::size_t size) {
	if (size <= buffer.getSize()) return;
	dbLog(dbg::LOG_DEBUG, "Resizing acceleration structure scratch buffer to ", size, " bytes.");
	buffer = VulkanBuffer(nri, size,
						  BufferUsage::BUFFER_USAGE_STORAGE | BufferUsage::BUFFER_USAGE_TRANSFER_SRC |
							  BufferUsage::BUFFER_USAGE_TRANSFER_DST);
	allocation =
		VulkanAllocation(nri, buffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	buffer.bindMemory(allocation, 0);
}

VulkanBuffer &VulkanMemoryCache::getAccelerationStructureScratch(VulkanNRI									&nri,
																 vk::AccelerationStructureBuildSizesInfoKHR &sizeInfo) {
	assureSize(nri, sizeInfo.buildScratchSize);
	return buffer;
}

VulkanMemoryCache &VulkanMemoryCache::getInstance() {
	static VulkanMemoryCache instance;
	return instance;
}

void VulkanMemoryCache::destroy() {
	getInstance().buffer	 = VulkanBuffer(nullptr);
	getInstance().allocation = VulkanAllocation(nullptr);
}

std::unique_ptr<Window> VulkanNRI::createGLFWWindow(GLFWwindow *glfwWindow) {
	vk::SurfaceKHR surface;
	{
		VkSurfaceKHR cSurface;
		VkResult	 result = glfwCreateWindowSurface(*instance, glfwWindow, nullptr, &cSurface);
		if (result != VK_SUCCESS) {
			THROW_RUNTIME_ERR(std::format("Failed to create Vulkan surface for GLFW window: {}", int(result)));
		}
		surface = vk::SurfaceKHR(cSurface);
	}
	auto window = std::make_unique<VulkanWindow>(*this);
	window->setSurface(vk::raii::SurfaceKHR(instance, surface));

	vk::Bool32 presentSupport =
		physicalDevice.getSurfaceSupportKHR(queueFamilyIndices.graphicsFamily.value(), *window->getSurface());
	if (!presentSupport) { throw std::runtime_error("Selected GPU does not support presentation to the surface!"); }

	uint32_t width;
	uint32_t height;
	window->createSwapChain(width, height);

	vk::raii::Queue presentQueue = vk::raii::Queue(device, queueFamilyIndices.graphicsFamily.value(), 0);
	window->getPresentQueue()	 = std::move(presentQueue);

	return window;
}

VulkanBLAS::VulkanBLAS(VulkanNRI &nri, VulkanBuffer &vertexBuffer, Format vertexFormat, std::size_t vertexOffset,
					   uint32_t vertexCount, std::size_t vertexStride, VulkanBuffer &indexBuffer, IndexType indexType,
					   std::size_t indexOffset)
	: nri(&nri), accelerationStructure(nullptr), asBuffer(nullptr), asMemory(nullptr), indexOffset(indexOffset) {
	assert(vertexFormat != Format::FORMAT_UNDEFINED);
	assert(vertexFormat < Format::_FORMAT_NUM);
	vk::Format	  vkVertexFormat = (vk::Format)nriFormat2vkFormat[static_cast<int>(vertexFormat)];
	vk::IndexType vkIndexType	 = (vk::IndexType)nriIndexType2vkIndexType[static_cast<int>(indexType)];

	vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
		vkVertexFormat, vertexBuffer.getAddress(), vertexStride, vertexCount, vkIndexType, indexBuffer.getAddress());
	this->tempBuildInfo				  = std::make_unique<TemporaryBuildInfo>();
	this->tempBuildInfo->vertexBuffer = &vertexBuffer;
	this->tempBuildInfo->indexBuffer  = &indexBuffer;

	this->tempBuildInfo->geometry = vk::AccelerationStructureGeometryKHR(vk::GeometryTypeKHR::eTriangles, trianglesData,
																		 vk::GeometryFlagBitsKHR::eOpaque);
	std::size_t primitiveCount	  = -1;
	if (indexType == IndexType::INDEX_TYPE_UINT16) primitiveCount = indexBuffer.getSize() / sizeof(uint16_t) / 3;
	else if (indexType == IndexType::INDEX_TYPE_UINT32) primitiveCount = indexBuffer.getSize() / sizeof(uint32_t) / 3;

	this->tempBuildInfo->buildRangeInfo =
		vk::AccelerationStructureBuildRangeInfoKHR(primitiveCount, vertexOffset, 0, 0);
	this->tempBuildInfo->buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR(
		vk::AccelerationStructureTypeKHR::eBottomLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
		vk::BuildAccelerationStructureModeKHR::eBuild, {}, this->accelerationStructure, 1,
		&this->tempBuildInfo->geometry);

	this->tempBuildInfo->sizeInfo = nri.getDevice().getAccelerationStructureBuildSizesKHR(
		vk::AccelerationStructureBuildTypeKHR::eDevice, this->tempBuildInfo->buildGeometryInfo,
		{this->tempBuildInfo->buildRangeInfo.primitiveCount});

	this->asBuffer = VulkanBuffer(nri, this->tempBuildInfo->sizeInfo.accelerationStructureSize,
								  BufferUsage::BUFFER_USAGE_ACCELERATION_STRUCTURE);
	this->asMemory =
		VulkanAllocation(nri, asBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	this->asBuffer.bindMemory(asMemory, 0);

	vk::AccelerationStructureCreateInfoKHR asCreateInfo({}, asBuffer.getBuffer(), 0,
														this->tempBuildInfo->sizeInfo.accelerationStructureSize,
														vk::AccelerationStructureTypeKHR::eBottomLevel);

	this->tempBuildInfo->scratchBuffer =
		VulkanBuffer(nri, this->tempBuildInfo->sizeInfo.buildScratchSize,
					 BufferUsage::BUFFER_USAGE_STORAGE | BufferUsage::BUFFER_USAGE_TRANSFER_SRC |
						 BufferUsage::BUFFER_USAGE_TRANSFER_DST);

	this->tempBuildInfo->scratchMemory = VulkanAllocation(
		nri,
		tempBuildInfo->scratchBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	this->tempBuildInfo->scratchBuffer.bindMemory(this->tempBuildInfo->scratchMemory, 0);

	this->tempBuildInfo->buildGeometryInfo.scratchData.setDeviceAddress(
		this->tempBuildInfo->scratchBuffer.getAddress());

	this->accelerationStructure = nri.getDevice().createAccelerationStructureKHR(asCreateInfo);
	this->tempBuildInfo->buildGeometryInfo.dstAccelerationStructure = this->accelerationStructure;
}

void VulkanBLAS::build(CommandBuffer &commandBuffer) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	assert(this->tempBuildInfo != nullptr);
	const auto &buildGeometryInfo = this->tempBuildInfo->buildGeometryInfo;
	const auto &buildRangeInfos	  = this->tempBuildInfo->buildRangeInfo;

	vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,
									vk::AccessFlagBits::eAccelerationStructureReadKHR);
	// buffer barrier to wait for vertex and index buffer transfer writes to finish
	vk::BufferMemoryBarrier vertexBufferBarrier(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eShaderRead, VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED, this->tempBuildInfo->vertexBuffer->getBuffer(), 0,
		this->tempBuildInfo->vertexBuffer->getSize());
	vk::BufferMemoryBarrier indexBufferBarrier(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eShaderRead, VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED, this->tempBuildInfo->indexBuffer->getBuffer(), 0,
		this->tempBuildInfo->indexBuffer->getSize());

	vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
										   vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {},
										   {memoryBarrier}, {vertexBufferBarrier, indexBufferBarrier}, {});

	vkCmdBuf.commandBuffer.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfos);

	vk::MemoryBarrier postBuildBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,
									   vk::AccessFlagBits::eAccelerationStructureReadKHR);
	vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
										   vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, {postBuildBarrier}, {},
										   {});
}

void VulkanBLAS::buildFinished() { this->tempBuildInfo.reset(); }

vk::DeviceAddress VulkanBLAS::getAddress() const {
	vk::AccelerationStructureDeviceAddressInfoKHR addressInfo(this->accelerationStructure);
	return nri->getDevice().getAccelerationStructureAddressKHR(addressInfo);
}

VulkanTLAS::VulkanTLAS(VulkanNRI &nri, const std::span<const BLAS *> &blases,
					   std::optional<std::span<glm::mat3x4>> transforms)
	: nri(&nri), as(nullptr), asBuffer(nullptr), asMemory(nullptr) {
	tempBuildInfo = std::make_unique<TemporaryBuildInfo>();

	tempBuildInfo->instanceUploadBuffer =
		VulkanBuffer(nri, sizeof(vk::AccelerationStructureInstanceKHR) * blases.size(),
					 BufferUsage::BUFFER_USAGE_TRANSFER_SRC | BufferUsage::BUFFER_USAGE_TRANSFER_DST);
	tempBuildInfo->instanceUploadMemory =
		VulkanAllocation(nri, tempBuildInfo->instanceUploadBuffer.getMemoryRequirements().setTypeRequest(
								  MemoryTypeRequest::MEMORY_TYPE_UPLOAD));
	tempBuildInfo->instanceUploadBuffer.bindMemory(tempBuildInfo->instanceUploadMemory, 0);
	{
		vk::AccelerationStructureInstanceKHR *data =
			(vk::AccelerationStructureInstanceKHR *)tempBuildInfo->instanceUploadBuffer.map(
				0, tempBuildInfo->instanceUploadBuffer.getSize());
		std::span<vk::AccelerationStructureInstanceKHR> instances{data,
																  data + tempBuildInfo->instanceUploadBuffer.getSize()};

		int i = 0;
		for (const auto &blas : blases) {
			auto								 &vulkanBLAS = static_cast<const VulkanBLAS &>(*blas);
			vk::AccelerationStructureInstanceKHR &instance	 = instances[i];
			if (transforms.has_value()) {
				auto &t = transforms.value()[i];
				std::memcpy(&instance.transform.matrix, &t, sizeof(glm::mat4x3));
			} else {
				glm::mat4x3 identity = glm::mat4x3(1.0f);
				std::memcpy(&instance.transform.matrix, &identity, sizeof(glm::mat4x3));
			}
			instance.instanceCustomIndex					= i;
			instance.mask									= 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = (VkGeometryInstanceFlagsKHR)vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable;
			instance.accelerationStructureReference = vulkanBLAS.getAddress();
			++i;
		}
		tempBuildInfo->instanceUploadBuffer.unmap();
	}

	// allocate instance buffer
	tempBuildInfo->instanceBuffer =
		VulkanBuffer(nri, tempBuildInfo->instanceUploadBuffer.getSize(),
					 BufferUsage::BUFFER_USAGE_TRANSFER_DST | BufferUsage::BUFFER_USAGE_ACCELERATION_STRUCTURE);
	tempBuildInfo->instanceMemory = VulkanAllocation(
		nri,
		tempBuildInfo->instanceBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	tempBuildInfo->instanceBuffer.bindMemory(tempBuildInfo->instanceMemory, 0);
	// will copy to it later

	// get size info
	vk::AccelerationStructureGeometryInstancesDataKHR instancesData(VK_FALSE,
																	tempBuildInfo->instanceBuffer.getAddress());
	tempBuildInfo->geometry = vk::AccelerationStructureGeometryKHR(vk::GeometryTypeKHR::eInstances, instancesData,
																   vk::GeometryFlagBitsKHR::eOpaque);

	tempBuildInfo->buildRangeInfo	 = vk::AccelerationStructureBuildRangeInfoKHR(blases.size(), 0, 0, 0);
	tempBuildInfo->buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR(
		vk::AccelerationStructureTypeKHR::eTopLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
		vk::BuildAccelerationStructureModeKHR::eBuild, {}, {}, 1, &tempBuildInfo->geometry, nullptr, nullptr);

	vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = nri.getDevice().getAccelerationStructureBuildSizesKHR(
		vk::AccelerationStructureBuildTypeKHR::eDevice, tempBuildInfo->buildGeometryInfo,
		{tempBuildInfo->buildRangeInfo.primitiveCount});

	// create AS buffer and AS itself
	this->asBuffer =
		VulkanBuffer(nri, sizeInfo.accelerationStructureSize, BufferUsage::BUFFER_USAGE_ACCELERATION_STRUCTURE);
	this->asMemory =
		VulkanAllocation(nri, asBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	this->asBuffer.bindMemory(asMemory, 0);

	vk::AccelerationStructureCreateInfoKHR asCreateInfo({}, asBuffer.getBuffer(), 0,
														sizeInfo.accelerationStructureSize);
	this->as = nri.getDevice().createAccelerationStructureKHR(asCreateInfo);
	tempBuildInfo->buildGeometryInfo.dstAccelerationStructure = this->as;

	// create scratch buffer
	tempBuildInfo->scratchBuffer =
		VulkanBuffer(nri, sizeInfo.buildScratchSize,
					 BufferUsage::BUFFER_USAGE_STORAGE | BufferUsage::BUFFER_USAGE_TRANSFER_SRC |
						 BufferUsage::BUFFER_USAGE_TRANSFER_DST);
	tempBuildInfo->scratchMemory = VulkanAllocation(
		nri,
		tempBuildInfo->scratchBuffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	tempBuildInfo->scratchBuffer.bindMemory(tempBuildInfo->scratchMemory, 0);

	tempBuildInfo->buildGeometryInfo.scratchData.setDeviceAddress(tempBuildInfo->scratchBuffer.getAddress());
}

void VulkanTLAS::build(CommandBuffer &commandBuffer) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	assert(this->tempBuildInfo != nullptr);
	vkCmdBuf.begin();

	tempBuildInfo->instanceBuffer.copyFrom(commandBuffer, tempBuildInfo->instanceUploadBuffer, 0, 0,
										   tempBuildInfo->instanceUploadBuffer.getSize());

	vk::BufferMemoryBarrier instanceBufferBarrier(
		vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureReadKHR, VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED, this->tempBuildInfo->instanceBuffer.getBuffer(), 0,
		this->tempBuildInfo->instanceBuffer.getSize());
	vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
										   vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, {},
										   {instanceBufferBarrier}, {});

	vkCmdBuf.commandBuffer.buildAccelerationStructuresKHR({this->tempBuildInfo->buildGeometryInfo},
														  &this->tempBuildInfo->buildRangeInfo);

	vk::MemoryBarrier postBuildBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,
									   vk::AccessFlagBits::eAccelerationStructureReadKHR);
	vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
										   vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, {postBuildBarrier}, {},
										   {});
}

void VulkanTLAS::buildFinished() { this->tempBuildInfo.reset(); }

ResourceHandle VulkanTLAS::getHandle() const {
	if (handle == ResourceHandle::INVALID_HANDLE) {
		handle = nri->getDescriptorAllocator().addAccelerationStructureDescriptor(*this);
	}
	return handle;
}

std::unique_ptr<BLAS> VulkanNRI::createBLAS(Buffer &vertexBuffer, Format vertexFormat, std::size_t vertexOffset,
											uint32_t vertexCount, std::size_t vertexStride, Buffer &indexBuffer,
											IndexType indexType, std::size_t indexOffset) {
	return std::make_unique<VulkanBLAS>(*this, static_cast<VulkanBuffer &>(vertexBuffer), vertexFormat, vertexOffset,
										vertexCount, vertexStride, static_cast<VulkanBuffer &>(indexBuffer), indexType,
										indexOffset);
}

std::unique_ptr<TLAS> VulkanNRI::createTLAS(const std::span<const BLAS *>		 &blases,
											std::optional<std::span<glm::mat3x4>> transforms) {
	return std::make_unique<VulkanTLAS>(*this, blases, transforms);
}

std::unique_ptr<Allocation> VulkanNRI::allocateMemory(MemoryRequirements memoryRequirements) {
	return std::make_unique<VulkanAllocation>(*this, memoryRequirements);
}

std::unique_ptr<CommandBuffer> VulkanNRI::createCommandBuffer(const CommandPool &pool) {
	vk::CommandBufferAllocateInfo allocInfo(static_cast<const VulkanCommandPool &>(pool).commandPool,
											vk::CommandBufferLevel::ePrimary, 1);
	vk::raii::CommandBuffers	  buffers(device, allocInfo);
	return std::make_unique<VulkanCommandBuffer>(std::move(buffers[0]));
};

CommandQueue::SubmitKey VulkanCommandQueue::submit(CommandBuffer &commandBuffer) {
	auto &cmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	cmdBuf.end();
	vk::CommandBuffer vk = cmdBuf.commandBuffer;
	queue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &vk), nullptr);
	return 0;	  // TODO: fix this
}

void VulkanCommandQueue::wait(SubmitKey key) {
	static_cast<void>(key);		// TODO: fix this
	queue.waitIdle();
}

static int __asd = []() {
	Factory::getInstance().registerNRI(
		"Vulkan", [](CreateBits bits) -> std::unique_ptr<NRI> { return std::make_unique<VulkanNRI>(bits); });
	return 0;
}();
}	  // namespace nri
