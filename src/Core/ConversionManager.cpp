#include "ConversionManager.h"

void ConversionManager::setStrategy(std::unique_ptr<IMediaConverter> strategy) {
    currentStrategy = std::move(strategy);
}

void ConversionManager::startConversion(const std::string& source, const std::string& destination) {
    if (!currentStrategy) return;
    if (currentState == ConversionState::Processing) return; // Prevent double-execution

    // Reset progress metrics and update status state [cite: 78]
    currentState = ConversionState::Processing;
    currentProgress = 0.0f;

    // Spawn the background process using a lambda capturing the inputs [cite: 78, 129]
    workerThread = std::jthread([this, source, destination]() {
        // Execute the heavy processing loop
        bool success = currentStrategy->execute(source, destination, currentProgress);
        
        if (success) {
            currentState = ConversionState::Completed;
            currentProgress = 1.0f;
        } else {
            currentState = ConversionState::Failed;
        }
    });
}

float ConversionManager::getProgress() const {
    return currentProgress.load(); // Lock-free read for the UI thread [cite: 132, 136]
}

ConversionState ConversionManager::getState() const {
    return currentState.load();
}