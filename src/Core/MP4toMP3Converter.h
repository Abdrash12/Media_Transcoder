#pragma once
#include "IMediaConverter.h"
#include <string>
#include <atomic>

extern "C" {
    struct AVFormatContext;
    struct AVCodecContext;
    struct SwrContext;
    struct AVFrame;
    struct AVPacket;
    struct AVAudioFifo; 
}

class MP4ToMP3Converter : public IMediaConverter {
private:
    AVFormatContext* inputFormatCtx = nullptr;
    AVFormatContext* outputFormatCtx = nullptr;
    AVCodecContext* decoderCtx = nullptr;
    AVCodecContext* encoderCtx = nullptr;
    SwrContext* resampleCtx = nullptr;
    AVAudioFifo* audioFifo = nullptr;

    int audioStreamIndex = -1;
    int64_t totalDuration = 0;
    int64_t globalPts = 0; // Tracks precise audio timing

    bool openInput(const std::string& sourcePath);
    bool initDecoder();
    bool initEncoder(const std::string& destPath);
    bool initResampler();
    bool initFifo(); 
    bool processStream(std::atomic<float>& progress);
    void cleanup(); 

public:
    MP4ToMP3Converter() = default;
    ~MP4ToMP3Converter() override;

    bool execute(const std::string& sourcePath, 
                 const std::string& destPath, 
                 std::atomic<float>& progress) override;
};