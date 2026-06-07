#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <windows.h>
#include "Core/ConversionManager.h"
#include "Core/MP4ToMP3Converter.h"
extern "C" {
#include <libavutil/log.h>
}
#include "GUI/MainWindow.h"


namespace fs = std::filesystem;

//NATIVE WINDOWS FILE PICKER
std::string openFileDialog() {
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Video Files (*.mp4)\0*.mp4\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    // Force the dialog to only select existing files and return to the original directory
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return ofn.lpstrFile;
    }
    return "";
}

//TERMINAL UI PROGRESS BAR
void drawProgressBar(float progress) {
    int barWidth = 50;
    std::cout << "\r[";
    int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0f) << " %";
    std::cout.flush();
}


int main(int, char**) {
    MainWindow app;
    app.run();
    return 0;
}