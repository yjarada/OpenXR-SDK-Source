#include "camera/camera_capture.h"
#include "utils/timer.h"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "=== USB Camera Capture Test ===" << std::endl;
    
    CameraCapture camera;
    Timer timer;
    
    // Initialize camera with optimal settings for VR (low latency)
    // Try 120 FPS first, fall back to 60 if needed
    if (!camera.Initialize("/dev/video0", 3200, 1200, 120)) {
        std::cout << "120 FPS failed, trying 60 FPS..." << std::endl;
        if (!camera.Initialize("/dev/video0", 3200, 1200, 60)) {
            std::cerr << "Failed to initialize camera!" << std::endl;
            return -1;
        }
    }
    
    std::cout << "Camera ready: " << camera.GetWidth() << "x" << camera.GetHeight() 
              << " @ " << camera.GetFPS() << "fps" << std::endl;
    
    cv::Mat frame;
    int frameCount = 0;
    
    std::cout << "Starting capture test (press ESC to stop)..." << std::endl;
    
    timer.Start();
    
    while (true) {
        Timer frameTimer;
        frameTimer.Start();
        
        // Capture frame
        if (!camera.CaptureFrame(frame)) {
            std::cerr << "Failed to capture frame!" << std::endl;
            break;
        }
        
        frameTimer.Stop();
        frameCount++;
        
        // Display frame (this will open a window)
        cv::imshow("USB Camera Feed", frame);
        
        // Print timing info every 60 frames
        if (frameCount % 60 == 0) {
            timer.Stop();
            double avgFPS = frameCount / timer.GetElapsedMilliseconds() * 1000.0;
            std::cout << "Frame " << frameCount << " - Capture time: " 
                      << frameTimer.GetElapsedMilliseconds() << "ms, Avg FPS: " 
                      << avgFPS << std::endl;
        }
        
        // Check for ESC key
        char key = cv::waitKey(1) & 0xFF;
        if (key == 27) { // ESC key
            break;
        }
    }
    
    timer.Stop();
    double totalTime = timer.GetElapsedMilliseconds() / 1000.0;
    double avgFPS = frameCount / totalTime;
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Frames captured: " << frameCount << std::endl;
    std::cout << "Total time: " << totalTime << "s" << std::endl;
    std::cout << "Average FPS: " << avgFPS << std::endl;
    
    cv::destroyAllWindows();
    camera.Shutdown();
    
    return 0;
}