Llama Launcher is a lightweight and intuitive C++/Qt GUI wrapper for the llama-server. Specifically designed for Linux users (optimized for openSUSE Tumbleweed and KDE Plasma), it provides a seamless way to manage and run local Large Language Models (LLMs) in GGUF format without the need for complex command-line arguments.

Key Features

    Compact tabbed window: Model, Server, Advanced, Monitor, Log and Launcher tabs, with the Start/Stop button and the server status always visible below them.
    Model Management:
        Model list: scans a models folder (recursively) for .gguf files and remembers recently used models from elsewhere. Multimodal projectors (mmproj) and the extra parts of split models are hidden.
        Model info read straight from the GGUF header: name, size, layer count, native context length and, for Mixture-of-Experts models, the number of experts.
        VRAM check: warns when the model is larger than the free memory of the selected GPU.
        Per-model settings: context size, GPU layers, device, MoE, Flash Attention, every option on the Advanced tab and the extra arguments are remembered for each model separately and restored when you switch back.
    Intelligent Parameter Optimization:
        Auto GPU Offloading (-ngl auto): Automatically calculates and offloads the maximum possible number of model layers to VRAM for peak performance.
        Native Context Detection (-c 0): Automatically retrieves the model's native context size from its metadata (e.g., 131,072 tokens for Qwen2.5).
        Smart Resource Fitting (--fit on): Dynamically adjusts memory-related parameters to fit your specific hardware, ensuring stable operation without crashes.
        MoE experts on CPU (--cpu-moe / --n-cpu-moe N): for large MoE models on small GPUs, keeps the expert weights in system RAM and the rest in VRAM. On a 4 GB card this doubled prompt processing speed for a 35B-A3B model compared to leaving it to --fit.
        Flash Attention selector (Auto / On / Off, passed as -fa).
    Advanced tab (each option is a checkbox or field instead of a flag to remember):
        Memory and performance: KV cache type (f16 / q8_0 / q4_0, as -ctk/-ctv; a quantized cache does not work with Flash Attention forced off, and the launcher warns about that), KV cache in system RAM (--no-kv-offload), --mlock, --no-mmap, CPU threads (-t), batch / micro-batch (-b / -ub) and parallel slots (-np), each with an Auto checkbox.
        Chat: thinking mode Auto / On / Off (--reasoning) and a thinking budget (--reasoning-budget), Jinja chat template (unchecked passes --no-jinja), and the multimodal projector: an mmproj*.gguf file next to the model is detected and can be loaded (--mmproj), optionally kept on the CPU (--no-mmproj-offload).
        Default sampling: temperature, top-p, top-k and min-p, each sent only when checked; clients can still override them per request.
        Other: Context Shift, Prometheus metrics endpoint (--metrics), skip the warm-up run (--no-warmup).
        Flags typed into Extra Arguments that now have their own control (e.g. -ctk q8_0 -ctv q8_0 --jinja -t 6) are moved into it automatically; anything else stays as typed.
    Monitoring (Monitor tab), read from /proc, amdgpu sysfs and the server itself, without root or extra packages:
        Charts for the last 5 minutes: CPU and RAM (whole system and llama-server), AMD GPU load with temperature, power and clock, VRAM with the shared system memory (GTT), and generation speed. Hover a chart to read the values at that moment.
        Live status: prompt processing progress and speed, generation speed in tokens per second, context in use, RAM used by llama-server, and the final speeds of the last request.
        Generation speed and context use come from llama-server's /slots endpoint (enabled by default), prompt progress and final speeds from its log. A sleeping (runtime-suspended) GPU is reported idle and is not woken up.
        Sampling stops while the window is in the tray and the server is not running.
    Multi-GPU & Custom Builds:
        llama-server path: use the system binary from PATH or point the launcher at your own build (e.g. a fresh llama.cpp Vulkan build). The detected version is shown under the field.
        Device selector: lists the devices reported by llama-server --list-devices and passes the choice as --device. Useful on machines with an integrated + discrete GPU, where offloading to both is usually slower than using the discrete card alone. "CPU only" passes --device none.
    Deep KDE Plasma Integration:
        System Tray Support: Runs quietly in the background with a dedicated tray icon.
        Minimize to Tray: Closing the main window hides the application to the tray instead of terminating the server.
        Single instance: starting the launcher again (e.g. from the application menu) brings up the window of the one already running instead of starting a second server.
        The server never outlives the launcher: logging out or a kill signal stops it cleanly, and if the launcher is killed outright or crashes, the kernel stops the server too, so it does not keep the port, RAM and VRAM.
        Dynamic Tooltips: Hover over the tray icon to see real-time server status, the loaded model and its URL.
        Quick Access Menu: Start/Stop the server, open it in the browser or exit from the right-click context menu.
        Notifications when the server is ready, fails to start or stops unexpectedly (can be turned off).
        Launch at login: starts hidden in the tray (writes ~/.config/autostart/llama-launcher.desktop); optionally starts the server right away.
    Local Network Access:
        "Allow access from local network" makes the server listen on all interfaces, IPv4 and IPv6 (--host ::, or 0.0.0.0 when IPv6 is disabled), so mDNS names such as https://myhost.local:8080/ that resolve to an IPv6 address work too; the network address (e.g. https://192.168.1.10:8080/) is shown in the status line and copied by Copy URL.
        Optional API key (with a Generate button). It is passed to llama-server via the LLAMA_API_KEY environment variable, so it does not appear in the process list or the log. Clients send it as "Authorization: Bearer <key>"; the Web UI asks for it in its settings.
        The settings file is kept readable by the owner only, since it stores the key.
        HTTPS encrypts prompts, answers and the API key on the network (--ssl-cert-file / --ssl-key-file). It is always on while access from the local network is allowed, so the server never accepts plain HTTP from other machines; for local-only use it is optional ("Use HTTPS" on the Server tab). Both the distro llama-server and a build with LLAMA_OPENSSL=ON support it. The launcher creates a small local CA and a server certificate for localhost, the host name (also name.local) and every LAN address, using the openssl tool; they are kept in ~/.config/LlamaLauncher/tls, the private keys readable by the owner only. When the address changes or the certificate is about to expire, a new server certificate is issued at start; the CA stays the same.
        To avoid browser warnings, import ~/.config/LlamaLauncher/tls/ca.pem once on each client device and trust it for websites (Firefox: Settings → Privacy & Security → Certificates → View Certificates → Authorities → Import; Chrome/Chromium on Linux: chrome://certificate-manager; Android: Settings → Security → Encryption & credentials → Install a certificate → CA certificate). The SHA-256 fingerprint shown on the Server tab lets you check that you imported the right file. Never share ca-key.pem. Alternatively, accept the browser warning once per device. API clients: pass the CA file (e.g. curl --cacert ca.pem, or SSL_CERT_FILE / REQUESTS_CA_BUNDLE for Python).
        You can also use your own certificate and key instead (for example from a CA you already run). The launcher itself trusts exactly the certificate the server was started with, for its health check and the monitor.
        The firewall must allow the port (on openSUSE: sudo firewall-cmd --add-port=8080/tcp --permanent && sudo firewall-cmd --reload, unless the interface is in a trusted zone). Do not expose the port to the internet; to reach the server from another network use a VPN such as Tailscale or WireGuard.
    Advanced LLM Controls:
        Status line with the server URL, plus Open in Browser and Copy URL buttons.
        Support for Context Shift (--context-shift) to handle infinite text generation efficiently.
        Option to disable the built-in Web UI to save system resources.
        Integrated log viewer (Log tab) with Copy and Clear buttons; it comes to the front when the server fails to start or stops unexpectedly.
        Hints from the server log: known llama-server warnings (automatic fitting skipped, Flash Attention turned off, no Context Shift support, out of memory) are explained above the log. Where there is one obvious fix, a link next to the hint applies it (e.g. "Turn off Context Shift"), and "Restart the server" starts it again with the new settings.
        Configurable server port (also used for the health check).
        Extra Arguments field (Advanced tab) for any other llama-server flags (e.g. --seed 42 --repeat-penalty 1.1), appended last so they can override defaults.
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

    On the Server tab, optionally choose the llama-server binary and the device (GPU) to use.
    On the Model tab, choose the models folder, then pick a model from the list (or Browse for a single .gguf file).
    Check the model info and VRAM hint; for large MoE models try "MoE experts on CPU".
    Enable Auto for Context Size and GPU Layers to let the launcher find the optimal settings.
    Optionally fine-tune the model on the Advanced tab (e.g. a q8_0 KV cache to fit a longer context into VRAM).
    Click Start Llama Server.
    By default, the server will be accessible at http://127.0.0.1:8080 (change the port on the Server tab).
    Watch speed and resource use on the Monitor tab; the server output is on the Log tab.

This project leverages the power of the llama.cpp HTTP server.
