#pragma once

#include "camera/camera_capture.h"
#include "utils/timer.h"

// IMPORTANT: Include platform headers FIRST, then Vulkan, then OpenXR
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <vulkan/vulkan.h>

// OpenXR headers
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// OpenCV
#include <opencv2/opencv.hpp>

// Standard library
#include <memory>
#include <vector>
#include <array>

class VRCameraApp {
public:
    VRCameraApp();
    ~VRCameraApp();
    
    bool Initialize();
    void Run();
    void Shutdown();

private:
    // =============================================================================
    // OpenXR Core Objects
    // =============================================================================
    XrInstance instance_ = XR_NULL_HANDLE;
    XrSystemId systemId_ = XR_NULL_SYSTEM_ID;
    XrSession session_ = XR_NULL_HANDLE;
    XrSpace appSpace_ = XR_NULL_HANDLE;
    XrSessionState sessionState_ = XR_SESSION_STATE_UNKNOWN;
    bool sessionRunning_ = false;
    
    // =============================================================================
    // Vulkan Objects (created via OpenXR)
    // =============================================================================
    VkInstance vkInstance_ = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice_ = VK_NULL_HANDLE;
    VkDevice vkDevice_ = VK_NULL_HANDLE;
    VkQueue vkQueue_ = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_ = 0;
    
    // Vulkan command objects
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    
    // =============================================================================
    // OpenXR View Configuration & Swapchains
    // =============================================================================
    XrViewConfigurationType viewConfigType_ = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    std::vector<XrViewConfigurationView> configViews_;
    std::vector<XrView> views_;
    
    struct Swapchain {
        XrSwapchain handle = XR_NULL_HANDLE;
        int32_t width = 0;
        int32_t height = 0;
        std::vector<XrSwapchainImageVulkan2KHR> images;
    };
    std::vector<Swapchain> swapchains_;  // One per eye
    
    // =============================================================================
    // Camera System
    // =============================================================================
    std::unique_ptr<CameraCapture> camera_;
    cv::Mat cameraFrame_;           // Full 1280x480 stereo frame
    cv::Mat leftEyeFrame_;          // Left 640x480
    cv::Mat rightEyeFrame_;         // Right 640x480
    
    // =============================================================================
    // Vulkan Texture Resources for Camera Upload
    // =============================================================================
    struct EyeTexture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };
    std::array<EyeTexture, 2> eyeTextures_;  // Left and right eye textures
    
    // Staging buffer for CPU→GPU uploads
    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory_ = VK_NULL_HANDLE;
    void* stagingBufferMapped_ = nullptr;
    
    // =============================================================================
    // Vulkan Rendering Pipeline
    // =============================================================================
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> descriptorSets_;  // One per eye
    
    // =============================================================================
    // Performance Monitoring
    // =============================================================================
    Timer frameTimer_;
    int frameCount_ = 0;
    
    // =============================================================================
    // OpenXR Initialization Methods
    // =============================================================================
    bool CreateInstance();
    bool CreateSystem();
    bool InitializeVulkan();
    bool CreateSession();
    bool CreateSwapchains();
    bool CreateSpaces();
    
    // =============================================================================
    // Vulkan Resource Creation
    // =============================================================================
    bool CreateVulkanResources();
    bool CreateEyeTextures();
    bool CreateStagingBuffer();
    bool CreateRenderPipeline();
    bool CreateDescriptorSets();
    
    // =============================================================================
    // Camera & Rendering Methods
    // =============================================================================
    void UpdateCamera();
    void UploadCameraTextures();
    void RenderFrame();
    bool RenderEyeTextures(XrTime displayTime);
    void RenderEye(int eyeIndex, const XrSwapchainImageVulkan2KHR& swapchainImage);
    
    // =============================================================================
    // Main Loop Methods
    // =============================================================================
    void PollEvents();
    bool IsSessionRunning() const;
    
    // =============================================================================
    // Vulkan Command Buffer Utilities
    // =============================================================================
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    // =============================================================================
    // Helper Methods
    // =============================================================================
    std::vector<const char*> GetRequiredExtensions();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void LogMessage(const std::string& message);
    void LogHeadsetRPY();  // Log Roll, Pitch, Yaw from headset
    void QuaternionToRPY(const XrQuaternionf& q, float& roll, float& pitch, float& yaw);
};