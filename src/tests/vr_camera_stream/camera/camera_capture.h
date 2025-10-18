#pragma once

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();
    
    // Initialize camera with device path
    bool Initialize(const std::string& devicePath = "/dev/video0", 
                   int width = 1280, int height = 480, int fps = 60);
    
    // Capture a frame (returns OpenCV Mat)
    bool CaptureFrame(cv::Mat& frame);
    
    // Get camera properties
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetFPS() const { return fps_; }
    
    void Shutdown();
    
private:
    std::unique_ptr<cv::VideoCapture> capture_;
    int width_, height_, fps_;
    bool initialized_;
};