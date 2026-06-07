#include "MainWindow.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <chrono>     
#include <cstring>

MainWindow::MainWindow() : inputPath(""), outputPath("") {
    SDL_Init(SDL_INIT_VIDEO);

   
    window = SDL_CreateWindow(
        "Meow Mix",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        960, 540,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string exeDir = std::string(buffer).substr(0, pos);

    std::string fontPath = exeDir + "/assets/dogicapixel.ttf";

    ImFont* loadedFont = nullptr;
    if (GetFileAttributesA(fontPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontDefault();
    } else {
        loadedFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
    }

    if (loadedFont) {
        io.FontDefault = loadedFont;
        io.FontGlobalScale = 1.0f;
        ImGui::GetStyle().ScaleAllSizes(1.0f);
    } else {
        io.FontDefault = io.Fonts->Fonts[0];
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;

    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(10, 10);
    style.ItemSpacing = ImVec2(14, 14);
    style.ItemInnerSpacing = ImVec2(10, 10);
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);              
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.08f, 0.16f, 1.0f); 
    style.Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0, 0, 0, 0);

    style.Colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.55f, 1.00f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.80f, 0.30f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.40f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.55f, 0.80f, 1.00f);

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.14f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.20f, 0.40f, 1.0f);

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}

MainWindow::~MainWindow() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void MainWindow::run() {
    bool running = true;
    std::string statusMessage = "Ready";
    ImVec4 statusColor = ImVec4(0.92f, 0.92f, 1.00f, 1.00f);

    const float btnW = 450.0f; 
    const float btnH = 50.0f;
    const float barH = 25.0f;

    static bool isRenaming = false;
    static char newFileNameBuf[256] = "";

    // Globals for drag logic
    static bool isDragging = false;
    static int dragOffsetX = 0, dragOffsetY = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        int w = 0, h = 0;
        SDL_GetWindowSize(window, &w, &h);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
        ImGui::Begin("Meow Mix", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

        // CUSTOM TITLE BAR BUTTONS (MINIMIZE & CLOSE) 
        ImGui::SetCursorPos(ImVec2(w - 95, 15)); // Top right corner
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        
        if (ImGui::Button("_", ImVec2(35, 35))) {
            SDL_MinimizeWindow(window); // Native minimize
        }
        ImGui::SameLine(0, 10);
        if (ImGui::Button("X", ImVec2(35, 35))) {
            running = false;            // Closes the app
        }
        ImGui::PopStyleVar();

        // GLOBAL DRAG LOGIC 
        // Only trigger drag if clicking the background, NOT hovering over our UI buttons
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
            int wx, wy, mx, my;
            SDL_GetWindowPosition(window, &wx, &wy);
            SDL_GetGlobalMouseState(&mx, &my); // Ask OS for absolute mouse pos
            dragOffsetX = mx - wx;
            dragOffsetY = my - wy;
            isDragging = true;
        }
        if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            int mx, my;
            SDL_GetGlobalMouseState(&mx, &my);
            SDL_SetWindowPosition(window, mx - dragOffsetX, my - dragOffsetY);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            isDragging = false;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float panelW = (btnW + 2 * 32.0f);
        float panelH = h * 0.78f;
        float panelX = (avail.x - panelW) * 0.5f;
        float panelY = (avail.y - panelH) * 0.5f;

        if (panelX > 0) ImGui::SetCursorPosX(panelX);
        if (panelY > 0) ImGui::SetCursorPosY(panelY);

        ImGui::BeginChild("Dashboard", ImVec2(panelW > 0 ? panelW : avail.x, panelH > 0 ? panelH : avail.y), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool isProcessing = (manager.getState() == ConversionState::Processing);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        if (ImGui::Button("SELECT VIDEO", ImVec2(btnW, btnH))) {
            OPENFILENAMEA ofn = {0};
            CHAR szFile[260] = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "MP4 Files\0*.mp4\0";
            if (GetOpenFileNameA(&ofn)) {
                this->inputPath = szFile;
                statusMessage = "Ready";
                statusColor = ImVec4(0.92f, 0.92f, 1.00f, 1.00f);
                isRenaming = false; 
            }
        }

        ImGui::Dummy(ImVec2(0, 14));

        ImGui::PushID("StartConversion");
        if (isProcessing) ImGui::BeginDisabled();
        
        if (ImGui::Button("START CONVERSION", ImVec2(btnW, btnH)) && !this->inputPath.empty()) {
            this->outputPath = this->inputPath.substr(0, this->inputPath.find_last_of(".")) + ".mp3";
            
            if (std::filesystem::exists(this->outputPath)) {
                ImGui::OpenPopup("Conflict!");
            } else {
                manager.setStrategy(std::make_unique<MP4ToMP3Converter>());
                manager.startConversion(this->inputPath, this->outputPath);
                statusMessage = "Converting...";
                statusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            }
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20)); 
        if (ImGui::BeginPopupModal("Conflict!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            
            if (!isRenaming) {
                ImGui::Text("File already exists!");
                ImGui::Dummy(ImVec2(0, 15));
                
                if (ImGui::Button("REPLACE", ImVec2(140, 40))) {
                    std::filesystem::remove(this->outputPath);
                    manager.setStrategy(std::make_unique<MP4ToMP3Converter>());
                    manager.startConversion(this->inputPath, this->outputPath);
                    statusMessage = "Converting (Overwritten)...";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine(0, 15);
                if (ImGui::Button("RENAME", ImVec2(140, 40))) {
                    isRenaming = true;
                    std::string base = this->inputPath.substr(this->inputPath.find_last_of("/\\") + 1);
                    base = base.substr(0, base.find_last_of("."));
                    std::strncpy(newFileNameBuf, base.c_str(), sizeof(newFileNameBuf));
                }
            } else {
                ImGui::Text("Enter new file name (without .mp3):");
                ImGui::Dummy(ImVec2(0, 5));
                
                ImGui::SetNextItemWidth(295); 
                ImGui::InputText("##newname", newFileNameBuf, IM_ARRAYSIZE(newFileNameBuf));
                ImGui::Dummy(ImVec2(0, 15));

                if (ImGui::Button("CONFIRM", ImVec2(140, 40))) {
                    std::string dir = this->inputPath.substr(0, this->inputPath.find_last_of("/\\") + 1);
                    this->outputPath = dir + std::string(newFileNameBuf) + ".mp3";
                    
                    manager.setStrategy(std::make_unique<MP4ToMP3Converter>());
                    manager.startConversion(this->inputPath, this->outputPath);
                    statusMessage = "Converting (Renamed)...";
                    statusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    isRenaming = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine(0, 15);
                if (ImGui::Button("CANCEL", ImVec2(140, 40))) {
                    isRenaming = false; 
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        if (isProcessing) ImGui::EndDisabled();
        ImGui::PopID();

        ImGui::Dummy(ImVec2(0, 18));

        ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.00f, 1.00f), "Input:");
        std::string displayPath = "(none)";
        if (!this->inputPath.empty()) {
            displayPath = this->inputPath.substr(this->inputPath.find_last_of("/\\") + 1);
        }
        ImGui::Text("%s", displayPath.c_str());

        ImGui::Dummy(ImVec2(0, 22));

        float prog = manager.getProgress();
        ImGui::ProgressBar(prog, ImVec2(-1.0f, barH));
        ImGui::Text("%3.0f%%", prog * 100.0f);
        ImGui::TextColored(statusColor, "%s", statusMessage.c_str());

        if (manager.getState() == ConversionState::Completed) {
            statusMessage = "Conversion Complete!";
            statusColor = ImVec4(0.20f, 1.00f, 0.20f, 1.00f);
        } else if (manager.getState() == ConversionState::Failed) {
            statusMessage = "Error: Transcoding Failed.";
            statusColor = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
        }

        ImGui::PopStyleVar(); 
        ImGui::EndChild();
        ImGui::PopStyleVar(); 
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 38, 26, 51, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
}