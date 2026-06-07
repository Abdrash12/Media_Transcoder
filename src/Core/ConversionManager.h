#pragma once
#include "IMediaConverter.h"
#include <memory>
#include <thread>
#include <atomic>
#include <string>

// Tracks operational state 
enum class ConversionState {
    Idle,
    Processing,
    Completed,
    Failed
};

class ConversionManager {
private:
    std::unique_ptr<IMediaConverter> currentStrategy; // Holds the polymorphic pointer
    std::jthread workerThread;                        // C++20 safe cooperative thread
    std::atomic<float> currentProgress{0.0f};         // Thread-safe progress tracking 
    std::atomic<ConversionState> currentState{ConversionState::Idle};

public:
    ConversionManager() = default;
    ~ConversionManager() = default; // std::jthread automatically joins cleanly on destruction

    // Exposes method to hot-swap the conversion strategy 
    void setStrategy(std::unique_ptr<IMediaConverter> strategy);

    // Asynchronous lifecycle control 
    void startConversion(const std::string& source, const std::string& destination);
    
    // Getters for the UI thread to poll
    float getProgress() const;
    ConversionState getState() const;
};