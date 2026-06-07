#pragma once
#include "../Core/ConversionManager.h"
#include "../Core/MP4ToMP3Converter.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

class MainWindow {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    ConversionManager manager;
    std::string inputPath;
    std::string outputPath; 

public:
    MainWindow();
    ~MainWindow();
    void run();
};