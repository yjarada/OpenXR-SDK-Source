#include "camera_capture.h"
#include <iostream>

CameraCapture::CameraCapture() 
    : capture_(nullptr), width_(0), height_(0), fps_(0), initialized_(false) {
}

CameraCapture::~CameraCapture() {
    Shutdown();
}

bool CameraCapture::Initialize(const std::string& devicePath, int width, int height, int fps) {
    std::cout << "Initializing camera: " << devicePath << " @ " << width << "x" << height << " " << fps << "fps" << std::endl;
    
    // Create VideoCapture object
    capture_ = std::make_unique<cv::VideoCapture>();
    
    // Open camera with V4L2 backend for better control
    if (!capture_->open(devicePath, cv::CAP_V4L2)) {
        std::cerr << "Failed to open camera: " << devicePath << std::endl;
        return false;
    }
    
    // IMPORTANT: Set MJPEG format FIRST for high FPS
    capture_->set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    
    // Set camera properties
    capture_->set(cv::CAP_PROP_FRAME_WIDTH, width);
    capture_->set(cv::CAP_PROP_FRAME_HEIGHT, height);
    capture_->set(cv::CAP_PROP_FPS, fps);
    
    // Set buffer size to 1 for lowest latency
    capture_->set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    // Verify actual settings
    width_ = static_cast<int>(capture_->get(cv::CAP_PROP_FRAME_WIDTH));
    height_ = static_cast<int>(capture_->get(cv::CAP_PROP_FRAME_HEIGHT));
    fps_ = static_cast<int>(capture_->get(cv::CAP_PROP_FPS));
    
    // Check what format we actually got
    int fourcc = static_cast<int>(capture_->get(cv::CAP_PROP_FOURCC));
    char fourccStr[5];
    fourccStr[0] = static_cast<char>(fourcc & 0xFF);
    fourccStr[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    fourccStr[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    fourccStr[3] = static_cast<char>((fourcc >> 24) & 0xFF);
    fourccStr[4] = '\0';
    
    std::cout << "Camera initialized: " << width_ << "x" << height_ << " @ " << fps_ << "fps" << std::endl;
    std::cout << "Format: " << fourccStr << ", Buffer size: " << capture_->get(cv::CAP_PROP_BUFFERSIZE) << std::endl;
    
    initialized_ = true;
    return true;
}

bool CameraCapture::CaptureFrame(cv::Mat& frame) {
    if (!initialized_ || !capture_) {
        return false;
    }
    
    return capture_->read(frame);
}

void CameraCapture::Shutdown() {
    if (capture_) {
        capture_->release();
        capture_.reset();
    }
    initialized_ = false;
    std::cout << "Camera shutdown complete" << std::endl;
}