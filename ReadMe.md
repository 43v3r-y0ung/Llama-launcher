Llama Launcher
Llama Launcher is a lightweight and intuitive C++/Qt GUI wrapper for the llama-server. Specifically designed for Linux users (optimized for openSUSE Tumbleweed and KDE Plasma), it provides a seamless way to manage and run local Large Language Models (LLMs) in GGUF format without the need for complex command-line arguments.
Key Features

    Model Management:
        Model list: scans a models folder (recursively) for .gguf files and remembers recently used models from elsewhere. Multimodal projectors (mmproj) and the extra parts of split models are hidden.
        Model info read straight from the GGUF header: name, size, layer count, native context length and, for Mixture-of-Experts models, the number of experts.
        VRAM check: warns when the model is larger than the free memory of the selected GPU.
        Per-model settings: context size, GPU layers, device, MoE, Flash Attention, context shift and extra arguments are remembered for each model separately and restored when you switch back.
    Intelligent Parameter Optimization:
        Auto GPU Offloading (-ngl auto): Automatically calculates and offloads the maximum possible number of model layers to VRAM for peak performance.
        Native Context Detection (-c 0): Automatically retrieves the model's native context size from its metadata (e.g., 131,072 tokens for Qwen2.5).
        Smart Resource Fitting (--fit on): Dynamically adjusts memory-related parameters to fit your specific hardware, ensuring stable operation without crashes.
        MoE experts on CPU (--cpu-moe / --n-cpu-moe N): for large MoE models on small GPUs, keeps the expert weights in system RAM and the rest in VRAM. On a 4 GB card this doubled prompt processing speed for a 35B-A3B model compared to leaving it to --fit.
        Flash Attention selector (Auto / On / Off, passed as -fa).
    Multi-GPU & Custom Builds:
        llama-server path: use the system binary from PATH or point the launcher at your own build (e.g. a fresh llama.cpp Vulkan build). The detected version is shown under the field.
        Device selector: lists the devices reported by llama-server --list-devices and passes the choice as --device. Useful on machines with an integrated + discrete GPU, where offloading to both is usually slower than using the discrete card alone. "CPU only" passes --device none.
    Deep KDE Plasma Integration:
        System Tray Support: Runs quietly in the background with a dedicated tray icon.
        Minimize to Tray: Closing the main window hides the application to the tray instead of terminating the server.
        Dynamic Tooltips: Hover over the tray icon to see real-time server status, the loaded model and its URL.
        Quick Access Menu: Start/Stop the server, open it in the browser or exit from the right-click context menu.
        Notifications when the server is ready, fails to start or stops unexpectedly (can be turned off).
        Launch at login: starts hidden in the tray (writes ~/.config/autostart/llama-launcher.desktop); optionally starts the server right away.
    Local Network Access:
        "Allow access from local network" makes the server listen on all interfaces (--host 0.0.0.0); the network address (e.g. http://192.168.1.10:8080/) is shown in the status line and copied by Copy URL.
        Optional API key (with a Generate button). It is passed to llama-server via the LLAMA_API_KEY environment variable, so it does not appear in the process list or the log. Clients send it as "Authorization: Bearer <key>"; the Web UI asks for it in its settings.
        The settings file is kept readable by the owner only, since it stores the key.
        The firewall must allow the port (on openSUSE: sudo firewall-cmd --add-port=8080/tcp --permanent && sudo firewall-cmd --reload, unless the interface is in a trusted zone). Do not expose the port to the internet; to reach the server from another network use a VPN such as Tailscale or WireGuard.
    Advanced LLM Controls:
        Status line with the server URL, plus Open in Browser and Copy URL buttons.
        Support for Context Shift (--context-shift) to handle infinite text generation efficiently.
        Option to disable the built-in Web UI to save system resources.
        Integrated log viewer to monitor the server's output in real-time.
        Configurable server port (also used for the health check).
        Extra Arguments field for any other llama-server flags (e.g. --jinja --temp 0.7), appended last so they can override defaults.
    Localization: English and Ukrainian; the language follows the system locale.

Technical Stack

    Language: C++
    Framework: Qt 6
    Build System: CMake (Qt LinguistTools for translations)
    Target OS: Linux (openSUSE, Arch, Fedora, etc.)

Installation & Setup
Prerequisites
The launcher needs a llama-server binary: either the distribution package or your own build.

# openSUSE Tumbleweed (package name: llamacpp)
sudo zypper install llamacpp

Building llama.cpp with Vulkan (optional)
Needs vulkan-devel, shaderc (provides glslc), cmake and a C++ compiler:

git clone https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
cmake -B build -DGGML_VULKAN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
build/bin/llama-server --list-devices

Then set the llama-server path in the launcher to llama.cpp/build/bin/llama-server.

Building from Source
You can build the project using CMake:

cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build-release --config Release

Installation & Desktop Integration
Install the binary, the icon and the application-menu entry in one step:

cmake --install build-release

This installs to the prefix chosen above:
    bin/Llama-launcher
    share/applications/llama-launcher.desktop
    share/icons/hicolor/scalable/apps/llama-launcher.svg
Use -DCMAKE_INSTALL_PREFIX=/usr/local (and sudo for the install step) for a system-wide install. To update, pull, rebuild and run the install command again.

Command line: Llama-launcher --autostart starts hidden in the tray (used by the login entry).

Translations: after changing UI strings run cmake --build build-release --target update_translations and translate translations/llama-launcher_uk.ts with Qt Linguist.

Settings are stored in ~/.config/LlamaLauncher/LlamaLauncher.conf (older versions used ~/.config/MyCompany/; those settings are imported automatically on first start).

How to Use

    Optionally choose the llama-server binary and the device (GPU) to use.
    Choose the models folder, then pick a model from the list (or Browse for a single .gguf file).
    Check the model info and VRAM hint; for large MoE models try "MoE experts on CPU".
    Enable Auto for Context Size and GPU Layers to let the launcher find the optimal settings.
    Click Start Llama Server.
    By default, the server will be accessible at http://127.0.0.1:8080 (change the port in the Port field).

This project leverages the power of the llama.cpp HTTP server.