# Meow Mix

A high-performance, native C++ media transcoder for converting MP4 video to MP3 audio. 

Built with a hardware-accelerated GUI to bypass the massive memory overhead of modern web-based frameworks (like Electron), Meow Mix offers maximum execution speed and a minimal system footprint. 

### Core Technologies
* **C++ (x64)** - Core architecture and processing.
* **SDL2 & Dear ImGui** - Hardware-accelerated, 60 FPS graphical interface.
* **FFmpeg** - Backend audio/video processing pipeline.
* **CMake** - Build system generation.

---

## Prerequisites

To build and run this project, you must be on **Windows** and use the **MSYS2 (MinGW-w64)** development environment.

1. Install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal.
3. Install the required toolchain and dependencies by running:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-sdl2 mingw-w64-ucrt-x86_64-ffmpeg pkgconf
```

---

## Build Instructions

1. **Clone the repository:**
```bash
git clone [https://github.com/abdrash12/Media_Transcoder.git](https://github.com/abdrash12/Media_Transcoder.git)
cd Media_Transcoder
```

2. **Create the build directory:**
```bash
mkdir build
cd build
```

3. **Generate the build files:**
*Note: It is highly recommended to enforce the MinGW generator and build in Release mode for maximum optimization.*
```bash
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
```

4. **Compile the application:**
```bash
mingw32-make
```

---

## Running the Application

Once compiled, the executable is located in your `build/` directory. You can run it directly from your terminal:

```bash
./MeowMix.exe
```

**⚠️ Important:** The application dynamically loads custom UI assets (e.g., `dogicapixel.ttf`). Ensure that the `assets/` directory remains in the parent folder relative to your `build/` directory, otherwise the application will fail to load the custom styling.

---

## Distributing to Other Machines

Because this is a dynamically linked C++ application, you cannot simply share the `.exe` file by itself. To run `MeowMix.exe` on a Windows machine that does not have MSYS2 installed, you must package it into a folder containing:

1. `MeowMix.exe`
2. The `assets/` folder.
3. The required MinGW `.dll` files (found in `C:\msys64\ucrt64\bin`). Copy the following next to your executable:
   * `SDL2.dll`
   * `libstdc++-6.dll`
   * `libgcc_s_seh-1.dll`
   * `libwinpthread-1.dll`
   * Any FFmpeg-related `.dll` files requested by Windows upon launch.
