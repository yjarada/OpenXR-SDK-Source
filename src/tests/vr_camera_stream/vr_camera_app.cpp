#include "vr_camera_app.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>

// Vulkan validation layers for debugging
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VRCameraApp::VRCameraApp() {
    camera_ = std::make_unique<CameraCapture>();
    
    // Initialize views vector for stereo
    views_.resize(2);
    for (auto& view : views_) {
        view = {XR_TYPE_VIEW};
    }
    
    LogMessage("VRCameraApp constructor completed");
}

VRCameraApp::~VRCameraApp() {
    Shutdown();
}

bool VRCameraApp::Initialize() {
    LogMessage("=== Initializing VR Camera Application ===");
    
    // Step 1: Initialize camera first (we know this works)
    LogMessage("Step 1: Initializing camera...");
    if (!camera_->Initialize("/dev/video0", 3200, 1200, 60)) {
        LogMessage("ERROR: Failed to initialize camera!");
        return false;
    }
    LogMessage("✓ Camera initialized successfully");
    
    // Step 2: Create OpenXR instance with Vulkan support
    LogMessage("Step 2: Creating OpenXR instance...");
    if (!CreateInstance()) {
        LogMessage("ERROR: Failed to create OpenXR instance!");
        return false;
    }
    LogMessage("✓ OpenXR instance created");
    
    // Step 3: Get system (HMD)
    LogMessage("Step 3: Getting OpenXR system...");
    if (!CreateSystem()) {
        LogMessage("ERROR: Failed to get OpenXR system!");
        return false;
    }
    LogMessage("✓ OpenXR system acquired");
    
    // Step 4: Initialize Vulkan through OpenXR
    LogMessage("Step 4: Initializing Vulkan via OpenXR...");
    if (!InitializeVulkan()) {
        LogMessage("ERROR: Failed to initialize Vulkan!");
        return false;
    }
    LogMessage("✓ Vulkan initialized via OpenXR");
    
    // Step 5: Create OpenXR session
    LogMessage("Step 5: Creating OpenXR session...");
    if (!CreateSession()) {
        LogMessage("ERROR: Failed to create OpenXR session!");
        return false;
    }
    LogMessage("✓ OpenXR session created");
    
    // Step 6: Create swapchains
    LogMessage("Step 6: Creating swapchains...");
    if (!CreateSwapchains()) {
        LogMessage("ERROR: Failed to create swapchains!");
        return false;
    }
    LogMessage("✓ Swapchains created");
    
    // Step 7: Create spaces
    LogMessage("Step 7: Creating spaces...");
    if (!CreateSpaces()) {
        LogMessage("ERROR: Failed to create spaces!");
        return false;
    }
    LogMessage("✓ Spaces created");
    
    // Step 8: Create Vulkan resources for camera textures
    LogMessage("Step 8: Creating Vulkan resources...");
    if (!CreateVulkanResources()) {
        LogMessage("ERROR: Failed to create Vulkan resources!");
        return false;
    }
    LogMessage("✓ Vulkan resources created");
    
    LogMessage("=== VR Camera Application Initialized Successfully! ===");
    return true;
}

bool VRCameraApp::CreateInstance() {
    // Request Vulkan extension (this is what works with SteamVR)
    std::vector<const char*> extensions = GetRequiredExtensions();
    
    XrApplicationInfo appInfo{};
    strcpy(appInfo.applicationName, "HelloXR");  // Use same name as hello_xr
    appInfo.applicationVersion = 1;
    strcpy(appInfo.engineName, "Custom");
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_API_VERSION_1_0;  // Use same version as hello_xr
    
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo = appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.enabledExtensionNames = extensions.data();
    
    XrResult result = xrCreateInstance(&createInfo, &instance_);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrCreateInstance failed!");
        return false;
    }
    
    return true;
}

std::vector<const char*> VRCameraApp::GetRequiredExtensions() {
    return {
        XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME  // This is what hello_xr uses successfully
    };
}

bool VRCameraApp::CreateSystem() {
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    
    XrResult result = xrGetSystem(instance_, &systemInfo, &systemId_);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrGetSystem failed!");
        return false;
    }
    
    // Log system properties
    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
    xrGetSystemProperties(instance_, systemId_, &systemProperties);
    LogMessage("System: " + std::string(systemProperties.systemName));
    
    return true;
}

bool VRCameraApp::InitializeVulkan() {
    // This follows the exact pattern from hello_xr that works with SteamVR
    
    // Step 1: Get Vulkan requirements
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
    XrResult result = xrGetInstanceProcAddr(instance_, "xrGetVulkanGraphicsRequirements2KHR",
                                          (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get xrGetVulkanGraphicsRequirements2KHR");
        return false;
    }
    
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
    result = pfnGetVulkanGraphicsRequirements2KHR(instance_, systemId_, &graphicsRequirements);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrGetVulkanGraphicsRequirements2KHR failed!");
        return false;
    }
    
    // Step 2: Create Vulkan instance using OpenXR
    PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
    result = xrGetInstanceProcAddr(instance_, "xrCreateVulkanInstanceKHR",
                                 (PFN_xrVoidFunction*)&pfnCreateVulkanInstanceKHR);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get xrCreateVulkanInstanceKHR");
        return false;
    }
    
    // Create VkInstanceCreateInfo structure
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VR Camera Stream";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    
    XrVulkanInstanceCreateInfoKHR createInfo{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    createInfo.systemId = systemId_;
    createInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    createInfo.vulkanCreateInfo = &instInfo;
    createInfo.vulkanAllocator = nullptr;
    
    VkResult vkResult;
    result = pfnCreateVulkanInstanceKHR(instance_, &createInfo, &vkInstance_, &vkResult);
    if (XR_FAILED(result) || vkResult != VK_SUCCESS) {
        LogMessage("ERROR: xrCreateVulkanInstanceKHR failed!");
        return false;
    }
    LogMessage("✓ Vulkan instance created via OpenXR");
    
    // Step 3: Get physical device from OpenXR
    PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2KHR = nullptr;
    result = xrGetInstanceProcAddr(instance_, "xrGetVulkanGraphicsDevice2KHR",
                                 (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsDevice2KHR);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get xrGetVulkanGraphicsDevice2KHR");
        return false;
    }
    
    XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    deviceGetInfo.systemId = systemId_;
    deviceGetInfo.vulkanInstance = vkInstance_;
    
    result = pfnGetVulkanGraphicsDevice2KHR(instance_, &deviceGetInfo, &vkPhysicalDevice_);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrGetVulkanGraphicsDevice2KHR failed!");
        return false;
    }
    LogMessage("✓ Vulkan physical device acquired from OpenXR");
    
    // Step 4: Create logical device using OpenXR
    PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;
    result = xrGetInstanceProcAddr(instance_, "xrCreateVulkanDeviceKHR",
                                 (PFN_xrVoidFunction*)&pfnCreateVulkanDeviceKHR);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get xrCreateVulkanDeviceKHR");
        return false;
    }
    
    XrVulkanDeviceCreateInfoKHR deviceCreateInfo{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    deviceCreateInfo.systemId = systemId_;
    deviceCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    deviceCreateInfo.vulkanPhysicalDevice = vkPhysicalDevice_;
    
    // Find queue family (same pattern as hello_xr)
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_, &queueFamilyCount, &queueFamilyProps[0]);

    uint32_t queueFamilyIndex = 0;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        // Only need graphics (not presentation) for draw queue
        if ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            queueFamilyIndex = i;
            break;
        }
    }
    
    // Create VkDeviceCreateInfo (same pattern as hello_xr)
    float queuePriorities = 0.0f;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriorities;
    
    std::vector<const char*> deviceExtensions;
    VkPhysicalDeviceFeatures features{};
    
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;
    deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
    deviceInfo.pEnabledFeatures = &features;

    deviceCreateInfo.vulkanCreateInfo = &deviceInfo;
    deviceCreateInfo.vulkanAllocator = nullptr;
    
    result = pfnCreateVulkanDeviceKHR(instance_, &deviceCreateInfo, &vkDevice_, &vkResult);
    if (XR_FAILED(result) || vkResult != VK_SUCCESS) {
        LogMessage("ERROR: xrCreateVulkanDeviceKHR failed!");
        return false;
    }
    LogMessage("✓ Vulkan logical device created via OpenXR");
    
    // Get the queue
    vkGetDeviceQueue(vkDevice_, queueFamilyIndex, 0, &vkQueue_);
    queueFamilyIndex_ = queueFamilyIndex;
    LogMessage("✓ Vulkan queue acquired");
    
    return true;
}

bool VRCameraApp::CreateSession() {
    // Create graphics binding with our Vulkan objects
    XrGraphicsBindingVulkan2KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
    graphicsBinding.instance = vkInstance_;
    graphicsBinding.physicalDevice = vkPhysicalDevice_;
    graphicsBinding.device = vkDevice_;
    graphicsBinding.queueFamilyIndex = queueFamilyIndex_;
    graphicsBinding.queueIndex = 0;
    
    XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &graphicsBinding;
    createInfo.systemId = systemId_;
    
    XrResult result = xrCreateSession(instance_, &createInfo, &session_);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrCreateSession failed!");
        return false;
    }
    
    return true;
}

bool VRCameraApp::CreateSwapchains() {
    // Get view configuration views
    uint32_t viewCount;
    XrResult result = xrEnumerateViewConfigurationViews(instance_, systemId_, viewConfigType_, 0, &viewCount, nullptr);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to enumerate view configuration views!");
        return false;
    }
    
    configViews_.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    result = xrEnumerateViewConfigurationViews(instance_, systemId_, viewConfigType_, viewCount, &viewCount, configViews_.data());
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get view configuration views!");
        return false;
    }
    
    // Initialize views vector for head tracking
    views_.resize(viewCount, {XR_TYPE_VIEW});
    
    LogMessage("Views: " + std::to_string(viewCount) + 
               ", Resolution: " + std::to_string(configViews_[0].recommendedImageRectWidth) + 
               "x" + std::to_string(configViews_[0].recommendedImageRectHeight));
    
    // Get supported swapchain formats
    uint32_t swapchainFormatCount;
    result = xrEnumerateSwapchainFormats(session_, 0, &swapchainFormatCount, nullptr);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to enumerate swapchain formats!");
        return false;
    }
    
    std::vector<int64_t> swapchainFormats(swapchainFormatCount);
    result = xrEnumerateSwapchainFormats(session_, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data());
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to get swapchain formats!");
        return false;
    }
    
    // Select color format (use the same logic as hello_xr)
    int64_t colorSwapchainFormat = 0;
    const std::vector<int64_t> supportedColorSwapchainFormats = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
    };
    
    for (int64_t format : supportedColorSwapchainFormats) {
        if (std::find(swapchainFormats.begin(), swapchainFormats.end(), format) != swapchainFormats.end()) {
            colorSwapchainFormat = format;
            break;
        }
    }
    
    if (colorSwapchainFormat == 0) {
        LogMessage("ERROR: No supported color swapchain format found!");
        return false;
    }
    LogMessage("✓ Selected swapchain format: " + std::to_string(colorSwapchainFormat));
    
    // Create swapchains for each eye
    swapchains_.resize(viewCount);
    for (uint32_t i = 0; i < viewCount; i++) {
        XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.format = colorSwapchainFormat;
        swapchainCreateInfo.width = configViews_[i].recommendedImageRectWidth;
        swapchainCreateInfo.height = configViews_[i].recommendedImageRectHeight;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.sampleCount = configViews_[i].recommendedSwapchainSampleCount;
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        
        result = xrCreateSwapchain(session_, &swapchainCreateInfo, &swapchains_[i].handle);
        if (XR_FAILED(result)) {
            LogMessage("ERROR: Failed to create swapchain " + std::to_string(i));
            return false;
        }
        
        swapchains_[i].width = swapchainCreateInfo.width;
        swapchains_[i].height = swapchainCreateInfo.height;
        
        // Get swapchain images
        uint32_t imageCount;
        result = xrEnumerateSwapchainImages(swapchains_[i].handle, 0, &imageCount, nullptr);
        if (XR_FAILED(result)) {
            LogMessage("ERROR: Failed to enumerate swapchain images!");
            return false;
        }
        
        swapchains_[i].images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
        result = xrEnumerateSwapchainImages(swapchains_[i].handle, imageCount, &imageCount,
                                          reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchains_[i].images.data()));
        if (XR_FAILED(result)) {
            LogMessage("ERROR: Failed to get swapchain images!");
            return false;
        }
        
        LogMessage("✓ Swapchain " + std::to_string(i) + " created: " + 
                   std::to_string(swapchains_[i].width) + "x" + std::to_string(swapchains_[i].height) +
                   " with " + std::to_string(imageCount) + " images");
    }
    
    return true;
}

bool VRCameraApp::CreateSpaces() {
    // Create app space
    XrReferenceSpaceCreateInfo createInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    createInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};  // Identity pose
    
    XrResult result = xrCreateReferenceSpace(session_, &createInfo, &appSpace_);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: Failed to create reference space!");
        return false;
    }
    
    return true;
}

bool VRCameraApp::CreateVulkanResources() {
    // Create command pool and command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex_;
    
    if (vkCreateCommandPool(vkDevice_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        LogMessage("ERROR: Failed to create command pool!");
        return false;
    }
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(vkDevice_, &allocInfo, &commandBuffer_) != VK_SUCCESS) {
        LogMessage("ERROR: Failed to allocate command buffer!");
        return false;
    }
    
    // Create staging buffer for camera texture uploads
    if (!CreateStagingBuffer()) {
        return false;
    }
    
    // Create textures for camera frames
    if (!CreateEyeTextures()) {
        return false;
    }
    
    LogMessage("✓ Vulkan resources created successfully");
    return true;
}

bool VRCameraApp::CreateStagingBuffer() {
    // Create staging buffer large enough for one eye frame (1600x1200x4 bytes for RGBA)
    VkDeviceSize bufferSize = 1600 * 1200 * 4;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(vkDevice_, &bufferInfo, nullptr, &stagingBuffer_) != VK_SUCCESS) {
        LogMessage("ERROR: Failed to create staging buffer!");
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice_, stagingBuffer_, &memRequirements);
    
    uint32_t memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryTypeIndex == UINT32_MAX) {
        LogMessage("ERROR: Failed to find suitable memory type for staging buffer");
        return false;
    }
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;    if (vkAllocateMemory(vkDevice_, &allocInfo, nullptr, &stagingBufferMemory_) != VK_SUCCESS) {
        LogMessage("ERROR: Failed to allocate staging buffer memory!");
        return false;
    }
    
    vkBindBufferMemory(vkDevice_, stagingBuffer_, stagingBufferMemory_, 0);
    
    // Map staging buffer memory
    vkMapMemory(vkDevice_, stagingBufferMemory_, 0, bufferSize, 0, &stagingBufferMapped_);
    
    LogMessage("✓ Staging buffer created and mapped");
    return true;
}

VkCommandBuffer VRCameraApp::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vkDevice_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VRCameraApp::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vkQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue_);

    vkFreeCommandBuffers(vkDevice_, commandPool_, 1, &commandBuffer);
}

bool VRCameraApp::CreateEyeTextures() {
    // Create textures for left and right eye (1600x1200 each)
    for (int eye = 0; eye < 2; eye++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = 1600;  // Half of camera width
        imageInfo.extent.height = 1200;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;  // Use same format as swapchain
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateImage(vkDevice_, &imageInfo, nullptr, &eyeTextures_[eye].image) != VK_SUCCESS) {
            LogMessage("ERROR: Failed to create eye texture " + std::to_string(eye));
            return false;
        }
        
        VkMemoryRequirements imgMemRequirements;
        vkGetImageMemoryRequirements(vkDevice_, eyeTextures_[eye].image, &imgMemRequirements);
        
        LogMessage("Image memory requirements: size=" + std::to_string(imgMemRequirements.size) + 
                  ", alignment=" + std::to_string(imgMemRequirements.alignment) +
                  ", memoryTypeBits=" + std::to_string(imgMemRequirements.memoryTypeBits));
        
        uint32_t imgMemoryTypeIndex = FindMemoryType(imgMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (imgMemoryTypeIndex == UINT32_MAX) {
            LogMessage("ERROR: Failed to find suitable memory type for eye texture " + std::to_string(eye));
            return false;
        }
        
        VkMemoryAllocateInfo imgAllocInfo{};
        imgAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imgAllocInfo.allocationSize = imgMemRequirements.size;
        imgAllocInfo.memoryTypeIndex = imgMemoryTypeIndex;
        
        LogMessage("Attempting allocation: size=" + std::to_string(imgAllocInfo.allocationSize) + 
                  ", memoryTypeIndex=" + std::to_string(imgAllocInfo.memoryTypeIndex));
        
        VkResult allocResult = vkAllocateMemory(vkDevice_, &imgAllocInfo, nullptr, &eyeTextures_[eye].memory);
        if (allocResult != VK_SUCCESS) {
            LogMessage("ERROR: vkAllocateMemory failed with result: " + std::to_string(allocResult));
            LogMessage("ERROR: Failed to allocate eye texture memory " + std::to_string(eye));
            return false;
        }
        
        vkBindImageMemory(vkDevice_, eyeTextures_[eye].image, eyeTextures_[eye].memory, 0);
        
        LogMessage("✓ Eye texture " + std::to_string(eye) + " created");
    }
    
    return true;
}

uint32_t VRCameraApp::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice_, &memProperties);
    
    LogMessage("Available memory types: " + std::to_string(memProperties.memoryTypeCount));
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        LogMessage("  Type " + std::to_string(i) + 
                  ": flags=" + std::to_string(memProperties.memoryTypes[i].propertyFlags) +
                  ", heap=" + std::to_string(memProperties.memoryTypes[i].heapIndex));
        
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            LogMessage("  -> Selected memory type " + std::to_string(i));
            return i;
        }
    }
    
    LogMessage("ERROR: No suitable memory type found for filter=" + std::to_string(typeFilter) + 
              ", properties=" + std::to_string(properties));
    
    // Try to find any compatible memory type, relaxing requirements
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)) {
            LogMessage("  -> Fallback to memory type " + std::to_string(i) + 
                      " with flags=" + std::to_string(memProperties.memoryTypes[i].propertyFlags));
            return i;
        }
    }
    
    // Return UINT32_MAX to indicate failure
    return UINT32_MAX;
}

void VRCameraApp::LogMessage(const std::string& message) {
    std::cout << "[VRCameraApp] " << message << std::endl;
}

void VRCameraApp::QuaternionToRPY(const XrQuaternionf& q, float& roll, float& pitch, float& yaw) {
    // Convert quaternion to Roll-Pitch-Yaw (in radians)
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    roll = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1.0f)
        pitch = copysignf(M_PI / 2.0f, sinp); // Use 90 degrees if out of range
    else
        pitch = asinf(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    yaw = atan2f(siny_cosp, cosy_cosp);
}

void VRCameraApp::LogHeadsetRPY() {
    if (views_.empty()) return;
    
    // Use the center eye view (average of left and right)
    // For simplicity, we'll just use the left eye view
    const XrPosef& headPose = views_[0].pose;
    
    float roll, pitch, yaw;
    QuaternionToRPY(headPose.orientation, roll, pitch, yaw);
    
    // Convert radians to degrees for display
    float rollDeg = roll * 180.0f / M_PI;
    float pitchDeg = pitch * 180.0f / M_PI;
    float yawDeg = yaw * 180.0f / M_PI;
    
    // Compact format with 1 decimal precision for faster reading
    char buffer[256];
    sprintf(buffer, "RPY: R=%.1f° P=%.1f° Y=%.1f°", rollDeg, pitchDeg, yawDeg);
    LogMessage(buffer);
}

void VRCameraApp::UpdateCamera() {
    if (!camera_->CaptureFrame(cameraFrame_)) {
        if (frameCount_ % 60 == 0) { // Log every 60 frames to avoid spam
            LogMessage("WARNING: Failed to capture camera frame");
        }
        return;
    }
    
    // Split stereo frame: left half and right half
    int width = cameraFrame_.cols / 2;  // 1600 pixels
    int height = cameraFrame_.rows;     // 1200 pixels
    
    leftEyeFrame_ = cameraFrame_(cv::Rect(0, 0, width, height));        // Left 1600x1200
    rightEyeFrame_ = cameraFrame_(cv::Rect(width, 0, width, height));   // Right 1600x1200
    
    frameCount_++;
    
    // Debug log every 60 frames for camera info
    if (frameCount_ % 60 == 0) {
        // LogMessage("Camera frame " + std::to_string(frameCount_) + ": " + 
        //           std::to_string(cameraFrame_.cols) + "x" + std::to_string(cameraFrame_.rows));
    }
    
    // Log headset orientation every 5 frames for very fast updates
    if (frameCount_ % 5 == 0) {
        LogHeadsetRPY();
    }
}

void VRCameraApp::UploadCameraTextures() {
    // Upload camera textures to GPU for both eyes
    std::array<cv::Mat*, 2> eyeFrames = {{&leftEyeFrame_, &rightEyeFrame_}};
    
    for (int eye = 0; eye < 2; eye++) {
        cv::Mat* frame = eyeFrames[eye];
        if (frame->empty()) {
            continue;
        }
        
        // Convert BGR to RGBA format
        cv::Mat rgbaFrame;
        cv::cvtColor(*frame, rgbaFrame, cv::COLOR_BGR2RGBA);
        
        // Copy data to staging buffer
        size_t imageSize = rgbaFrame.rows * rgbaFrame.cols * 4; // RGBA = 4 bytes per pixel
        memcpy(stagingBufferMapped_, rgbaFrame.data, imageSize);
        
        // Begin command buffer for texture upload
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        
        // Transition image layout to transfer destination
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = eyeTextures_[eye].image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {1600, 1200, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer_, eyeTextures_[eye].image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        // Transition image layout to shader read optimal
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        EndSingleTimeCommands(commandBuffer);
    }
}

void VRCameraApp::Run() {
    LogMessage("=== Starting VR Camera Main Loop ===");
    frameTimer_.Start();
    
    // Keep running until we get an explicit exit signal
    bool shouldExit = false;
    
    while (!shouldExit) {
        PollEvents();
        
        // Check for exit conditions
        if (sessionState_ == XR_SESSION_STATE_EXITING) {
            shouldExit = true;
            break;
        }
        
        // Render if session is ready or focused (like Hello_XR does)
        if (sessionState_ == XR_SESSION_STATE_READY || sessionState_ == XR_SESSION_STATE_FOCUSED) {
            static bool loggedRendering = false;
            if (!loggedRendering) {
                LogMessage("=== STARTING RENDERING - Session State: " + std::to_string(sessionState_) + " ===");
                loggedRendering = true;
            }
            
            UpdateCamera();
            UploadCameraTextures();
            RenderFrame();
            
            // Log performance every 120 frames (2 seconds at 60fps)
            if (frameCount_ % 120 == 0) {
                frameTimer_.Stop();
                double avgFPS = frameCount_ / frameTimer_.GetElapsedMilliseconds() * 1000.0;
                // LogMessage("Frame " + std::to_string(frameCount_) + " - Average FPS: " + std::to_string(avgFPS));
            }
        } else {
            // Log current state periodically when not rendering
            static int stateLogCounter = 0;
            if (++stateLogCounter % 300 == 0) { // Every ~5 seconds
                LogMessage("Waiting for READY/FOCUSED state. Current state: " + std::to_string(sessionState_));
            }
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LogMessage("=== VR Camera Main Loop Ended ===");
}

void VRCameraApp::PollEvents() {
    XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(instance_, &eventData) == XR_SUCCESS) {
        switch (eventData.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                XrEventDataSessionStateChanged* stateEvent = 
                    reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
                sessionState_ = stateEvent->state;
                
                LogMessage("Session state changed to: " + std::to_string(sessionState_));
                
                if (sessionState_ == XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                    beginInfo.primaryViewConfigurationType = viewConfigType_;
                    XrResult result = xrBeginSession(session_, &beginInfo);
                    if (XR_SUCCEEDED(result)) {
                        sessionRunning_ = true;
                        LogMessage("✓ Session started successfully");
                    } else {
                        LogMessage("ERROR: Failed to begin session!");
                    }
                } else if (sessionState_ == XR_SESSION_STATE_STOPPING) {
                    sessionRunning_ = false;
                    xrEndSession(session_);
                    LogMessage("✓ Session ended");
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                LogMessage("Instance loss pending - shutting down");
                sessionRunning_ = false;
                break;
            }
            default:
                // Handle all other event types (including XR_STRUCTURE_TYPE_MAX_ENUM)
                break;
        }
        eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

bool VRCameraApp::IsSessionRunning() const {
    return sessionRunning_ && 
           sessionState_ != XR_SESSION_STATE_STOPPING && 
           sessionState_ != XR_SESSION_STATE_EXITING;
}

void VRCameraApp::RenderFrame() {
    static int renderFrameCount = 0;
    if (renderFrameCount++ % 60 == 0) {
        // LogMessage("RenderFrame called - count: " + std::to_string(renderFrameCount));
    }
    
    // Begin frame
    XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    
    XrResult result = xrWaitFrame(session_, &frameWaitInfo, &frameState);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrWaitFrame failed");
        return;
    }

    XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    result = xrBeginFrame(session_, &frameBeginInfo);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrBeginFrame failed");
        return;
    }

    std::vector<XrCompositionLayerBaseHeader*> layers;
    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::vector<XrCompositionLayerProjectionView> projectionLayerViews;

    if (frameState.shouldRender == XR_TRUE) {
        // Render only if we should render
        if (RenderEyeTextures(frameState.predictedDisplayTime)) {
            // Set up projection layer
            projectionLayerViews.resize(configViews_.size());
            layer.space = appSpace_;
            layer.layerFlags = 0;
            layer.viewCount = (uint32_t)projectionLayerViews.size();
            layer.views = projectionLayerViews.data();

            for (uint32_t i = 0; i < configViews_.size(); i++) {
                const Swapchain& viewSwapchain = swapchains_[i];
                projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
                projectionLayerViews[i].pose = views_[i].pose;
                projectionLayerViews[i].fov = views_[i].fov;
                projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
                projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
                projectionLayerViews[i].subImage.imageRect.extent = {(int32_t)viewSwapchain.width, (int32_t)viewSwapchain.height};
            }

            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
        }
    }

    // End frame
    XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
    frameEndInfo.displayTime = frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = (uint32_t)layers.size();
    frameEndInfo.layers = layers.data();

    result = xrEndFrame(session_, &frameEndInfo);
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrEndFrame failed");
    }
}

bool VRCameraApp::RenderEyeTextures(XrTime displayTime) {
    // Locate views (head tracking)
    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCapacityInput = (uint32_t)views_.size();
    uint32_t viewCountOutput;
    
    XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    viewLocateInfo.viewConfigurationType = viewConfigType_;
    viewLocateInfo.displayTime = displayTime;
    viewLocateInfo.space = appSpace_;
    
    XrResult result = xrLocateViews(session_, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, views_.data());
    if (XR_FAILED(result)) {
        LogMessage("ERROR: xrLocateViews failed");
        return false;
    }
    
    // Render each eye
    for (uint32_t eyeIndex = 0; eyeIndex < viewCountOutput; eyeIndex++) {
        // Acquire swapchain image
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t swapchainImageIndex;
        result = xrAcquireSwapchainImage(swapchains_[eyeIndex].handle, &acquireInfo, &swapchainImageIndex);
        if (XR_FAILED(result)) {
            LogMessage("ERROR: xrAcquireSwapchainImage failed for eye " + std::to_string(eyeIndex));
            return false;
        }
        
        // Wait for swapchain image to be ready
        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(swapchains_[eyeIndex].handle, &waitInfo);
        if (XR_FAILED(result)) {
            LogMessage("ERROR: xrWaitSwapchainImage failed for eye " + std::to_string(eyeIndex));
            return false;
        }
        
        // Get the swapchain image
        const XrSwapchainImageVulkan2KHR& swapchainImage = swapchains_[eyeIndex].images[swapchainImageIndex];
        
        // Render to this eye's swapchain image
        RenderEye(eyeIndex, swapchainImage);
        
        // Release swapchain image
        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        result = xrReleaseSwapchainImage(swapchains_[eyeIndex].handle, &releaseInfo);
        if (XR_FAILED(result)) {
            LogMessage("ERROR: xrReleaseSwapchainImage failed for eye " + std::to_string(eyeIndex));
            return false;
        }
    }
    
    return true;
}

void VRCameraApp::RenderEye(int eyeIndex, const XrSwapchainImageVulkan2KHR& swapchainImage) {
    // Create a command buffer for this eye's rendering
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    
    // Clear the swapchain image to a dark blue color (for now)
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    
    // Transition swapchain image to transfer destination
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = swapchainImage.image;
    barrier.subresourceRange = subresourceRange;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    // Scale the camera image to fill more of the swapchain
    uint32_t swapchainWidth = swapchains_[eyeIndex].width;   // 2468
    uint32_t swapchainHeight = swapchains_[eyeIndex].height; // 2740
    
    // Scale down the high-res camera image to fit in VR display
    // Camera: 1600x1200 -> Scaled: 1600x1200 (fits well in 2468x2740 display)
    uint32_t scaledWidth = 1600;   // Use original camera resolution
    uint32_t scaledHeight = 1200;  // Use original camera resolution
    
    // Center the scaled image in the swapchain
    uint32_t offsetX = (swapchainWidth - scaledWidth) / 2;   // (2468-1600)/2 = 434
    uint32_t offsetY = (swapchainHeight - scaledHeight) / 2; // (2740-1200)/2 = 770
    
    // Transition eye texture to transfer source
    VkImageMemoryBarrier srcBarrier{};
    srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    srcBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    srcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.image = eyeTextures_[eyeIndex].image;
    srcBarrier.subresourceRange = subresourceRange;
    srcBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &srcBarrier);
    
    // Use blit to scale the image instead of copy
    VkImageBlit blitRegion{};
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcOffsets[0] = {0, 0, 0};
    blitRegion.srcOffsets[1] = {1600, 1200, 1};  // Source: full 1600x1200 camera image
    
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.mipLevel = 0;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstOffsets[0] = {(int32_t)offsetX, (int32_t)offsetY, 0};
    blitRegion.dstOffsets[1] = {(int32_t)(offsetX + scaledWidth), (int32_t)(offsetY + scaledHeight), 1};
    
    // Blit (scale) the eye texture to the swapchain image
    vkCmdBlitImage(commandBuffer,
                   eyeTextures_[eyeIndex].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapchainImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blitRegion, VK_FILTER_LINEAR);
    
    // Transition swapchain image to color attachment optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    // Transition eye texture back to shader read optimal
    srcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    srcBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &srcBarrier);
    
    EndSingleTimeCommands(commandBuffer);
}

void VRCameraApp::Shutdown() {
    LogMessage("=== Shutting down VR Camera Application ===");
    
    // Clean up Vulkan resources
    if (stagingBufferMapped_) {
        vkUnmapMemory(vkDevice_, stagingBufferMemory_);
        stagingBufferMapped_ = nullptr;
    }
    
    if (stagingBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkDevice_, stagingBuffer_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
    }
    
    if (stagingBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(vkDevice_, stagingBufferMemory_, nullptr);
        stagingBufferMemory_ = VK_NULL_HANDLE;
    }
    
    for (auto& eyeTexture : eyeTextures_) {
        if (eyeTexture.image != VK_NULL_HANDLE) {
            vkDestroyImage(vkDevice_, eyeTexture.image, nullptr);
            eyeTexture.image = VK_NULL_HANDLE;
        }
        if (eyeTexture.memory != VK_NULL_HANDLE) {
            vkFreeMemory(vkDevice_, eyeTexture.memory, nullptr);
            eyeTexture.memory = VK_NULL_HANDLE;
        }
    }
    
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(vkDevice_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
    
    // Clean up OpenXR
    for (auto& swapchain : swapchains_) {
        if (swapchain.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(swapchain.handle);
        }
    }
    
    if (appSpace_ != XR_NULL_HANDLE) {
        xrDestroySpace(appSpace_);
        appSpace_ = XR_NULL_HANDLE;
    }
    
    if (session_ != XR_NULL_HANDLE) {
        xrDestroySession(session_);
        session_ = XR_NULL_HANDLE;
    }
    
    if (vkDevice_ != VK_NULL_HANDLE) {
        vkDestroyDevice(vkDevice_, nullptr);
        vkDevice_ = VK_NULL_HANDLE;
    }
    
    if (vkInstance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(vkInstance_, nullptr);
        vkInstance_ = VK_NULL_HANDLE;
    }
    
    if (instance_ != XR_NULL_HANDLE) {
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
    
    // Shutdown camera
    if (camera_) {
        camera_->Shutdown();
    }
    
    LogMessage("=== Shutdown complete ===");
}