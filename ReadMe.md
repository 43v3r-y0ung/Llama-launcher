lama Launcher
Llama Launcher is a lightweight and intuitive C++/Qt GUI wrapper for the llama-server. Specifically designed for Linux users (optimized for openSUSE Tumbleweed and KDE Plasma), it provides a seamless way to manage and run local Large Language Models (LLMs) in GGUF format without the need for complex command-line arguments.
Key Features

    Intelligent Parameter Optimization:
        Auto GPU Offloading (-ngl auto): Automatically calculates and offloads the maximum possible number of model layers to VRAM for peak performance.
        Native Context Detection (-c 0): Automatically retrieves the model's native context size from its metadata (e.g., 131,072 tokens for Qwen2.5).
        Smart Resource Fitting (--fit on): Dynamically adjusts memory-related parameters to fit your specific hardware, ensuring stable operation without crashes.
    Deep KDE Plasma Integration:
        System Tray Support: Runs quietly in the background with a dedicated tray icon.
        Minimize to Tray: Closing the main window hides the application to the tray instead of terminating the server.
        Dynamic Tooltips: Hover over the tray icon to see real-time server status and the name of the currently loaded model.
        Quick Access Menu: Start/Stop the server or exit the application directly from the right-click context menu.
    Advanced LLM Controls:
        Support for Context Shift (--context-shift) to handle infinite text generation efficiently.
        Option to disable the built-in Web UI to save system resources.
        Integrated log viewer to monitor the server's output in real-time.

Technical Stack

    Language: C++
    Framework: Qt 6
    Build System: CMake
    Target OS: Linux (openSUSE, Arch, Fedora, etc.)

Installation & Setup
Prerequisites
The launcher requires the system-installed llama-server. Ensure it is available on your machine:

# For openSUSE Tumbleweed
sudo zypper install llama.cpp-server

Building from Source
You can build the project using CMake:

cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release

Desktop Integration
To integrate the launcher into your application menu, create a .desktop file at ~/.local/share/applications/llama-launcher.desktop:

[Desktop Entry]
Name=Llama Launcher
Comment=GUI for running local LLM models via llama-server
Exec=/path/to/your/bin/Llama-launcher
Icon=/path/to/your/icons/icon.svg
Type=Application
Categories=Development;Utility;

How to Use

    Browse for your model file (.gguf).
    Enable Auto for Context Size and GPU Layers to let the launcher find the optimal settings.
    Click Start Llama Server.
    By default, the server will be accessible at http://127.0.0.1:8080.

This project leverages the power of the llama.cpp HTTP server.