#pragma once
#include <string>
#include <atomic>

class IMediaConverter {
public:
    // Virtual destructor is mandatory for safe polymorphic deletion
    virtual ~IMediaConverter() = default;

    // The core execution method accepting paths and a thread-safe progress tracker 
    virtual bool execute(const std::string& sourcePath, 
                         const std::string& destPath, 
                         std::atomic<float>& progress) = 0;
};