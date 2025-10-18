#include "vr_camera_app.h"
#include <iostream>

int main() {
    std::cout << "=== VR Camera Streaming Application ===" << std::endl;
    std::cout << "Using OpenXR with Vulkan graphics" << std::endl;
    std::cout << "Target: HTC Vive Pro with SteamVR runtime" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    VRCameraApp app;
    
    if (!app.Initialize()) {
        std::cerr << "FATAL: Failed to initialize VR application!" << std::endl;
        return -1;
    }
    
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: Application error: " << e.what() << std::endl;
        return -1;
    }
    
    app.Shutdown();
    std::cout << "Application shutdown complete." << std::endl;
    
    return 0;
}