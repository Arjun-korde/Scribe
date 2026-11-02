# Tech Stack, Tools and Setup

> Working name: **Scribe** (placeholder, final name TBD). A Notepad-style text editor for Windows built with modern C++ and Qt 6 Widgets.

## 1. Tech Stack Summary

| Area | Choice | Notes |
|---|---|---|
| Language | C++20 | Concepts, `std::span`, `std::optional`, ranges, `std::jthread` where useful |
| UI framework | Qt 6 Widgets (6.5 LTS or newer) | Use an LTS release for stability |
| Build system | CMake (3.25+) with Ninja | No qmake |
| Compiler | MSVC (Visual Studio 2022 Build Tools) | Primary target. clang-cl optional |
| Dependency manager | vcpkg (manifest mode) | `vcpkg.json` in repo root |
| Unit testing | Qt Test (UI/Qt classes), GoogleTest (pure C++ logic) | Run through CTest |
| Static analysis | clang-tidy, cppcheck | Run in CI |
| Sanitizers | AddressSanitizer (MSVC `/fsanitize=address`) | Debug builds / CI job |
| Formatting | clang-format | `.clang-format` in repo root |
| Version control | Git + GitHub | |
| CI | GitHub Actions (`windows-latest`) | Build, test, static analysis, release artifacts |
| Installer | Inno Setup (simple) or WiX Toolset (MSI) | Start with Inno Setup |
| Windows APIs | Win32 (file sharing, DWM dark title bar, jump lists, shell integration) | Via `windows.h`, `dwmapi`, `shobjidl` |
| Speech-to-text (later) | whisper.cpp (offline, MIT) | Behind an `ISpeechEngine` interface |
| Audio capture (later) | Qt Multimedia (`QAudioSource`) | 16 kHz mono PCM |
| Encoding detection | Custom BOM and heuristic detector, optionally uchardet | Needs unit tests |
| Settings | `QSettings` (INI/registry) and JSON files | Snippets, macros, themes are JSON |
| Documentation | Markdown in `docs/` | Optional Doxygen later |

## 2. Required Tools and Packages

### 2.1 Required for v0.1

| Tool | Purpose | Minimum version |
|---|---|---|
| Windows 10/11 x64 | Dev and target OS | Windows 10 21H2+ |
| Visual Studio 2022 Build Tools (or full VS) | MSVC compiler, Windows SDK | 17.x, workload "Desktop development with C++" |
| Qt 6 (MSVC 2022 64-bit kit) | UI framework | 6.5 LTS or newer |
| CMake | Build configuration | 3.25 |
| Ninja | Fast build backend | 1.11 |
| Git | Source control | 2.40 |
| vcpkg | C++ package manager | latest |
| Qt Creator, Visual Studio, or VS Code | IDE (pick one) | latest |

### 2.2 Required later

| Tool | Needed from | Purpose |
|---|---|---|
| Inno Setup 6 | v1.0 | Build the installer |
| clang-tidy, cppcheck | CI setup (v0.2) | Static analysis |
| clang-format | v0.1 | Consistent style |
| whisper.cpp + a ggml model file | Speech phase | Offline speech-to-text |
| Qt Multimedia module | Speech phase | Microphone capture |

### 2.3 vcpkg dependencies (`vcpkg.json`)

Qt itself is installed with the Qt Online Installer (more reliable on Windows than building it from vcpkg). vcpkg is used for the smaller libraries:

```json
{
  "name": "scribe",
  "version-string": "0.1.0",
  "dependencies": [
    "gtest",
    "nlohmann-json"
  ]
}
```

Add later: `uchardet` (encoding detection, optional), `whisper-cpp` (speech phase, or vendor it as a git submodule).

## 3. Setup Guide (Windows)

### Step 1: Install Visual Studio 2022 Build Tools
1. Download from https://visualstudio.microsoft.com/downloads/ (Build Tools for Visual Studio 2022).
2. Select the workload **Desktop development with C++**.
3. Make sure these are ticked: MSVC v143 x64/x86 build tools, Windows 11 SDK, C++ CMake tools, C++ Clang tools (optional, for clang-tidy and clang-format).

### Step 2: Install Qt 6
1. Download the Qt Online Installer from https://www.qt.io/download-qt-installer-oss (a free Qt account is required).
2. Choose the **Custom installation** and select, under your chosen Qt 6 version (e.g. 6.8 LTS):
   - **MSVC 2022 64-bit**
   - **Qt Multimedia** (needed later for speech)
   - **Qt 5 Compatibility Module** (optional)
   - **Qt Debug Information Files** (useful for debugging)
3. Under **Developer and Designer Tools**, select Qt Creator, CMake, and Ninja (this is the easiest way to get them, but you may install them separately).
4. Note the install path, e.g. `C:\Qt\6.8.0\msvc2022_64`.

### Step 3: Install CMake, Ninja and Git (if not installed through Qt)
```powershell
winget install Kitware.CMake
winget install Ninja-build.Ninja
winget install Git.Git
```
Open a new terminal, then verify:
```powershell
cmake --version
ninja --version
git --version
```

### Step 4: Install vcpkg
```powershell
cd C:\
git clone https://github.com/microsoft/vcpkg.git
C:\vcpkg\bootstrap-vcpkg.bat
setx VCPKG_ROOT "C:\vcpkg"
```
Restart the terminal after `setx`.

### Step 5: Set environment variables
```powershell
setx CMAKE_PREFIX_PATH "C:\Qt\6.8.0\msvc2022_64"
```
Adjust the path to your Qt version. Optionally add `C:\Qt\6.8.0\msvc2022_64\bin` to `PATH` so Qt DLLs are found when you run from a terminal.

### Step 6: Install optional tools
```powershell
winget install JRSoftware.InnoSetup   # installer, needed at v1.0
winget install LLVM.LLVM              # clang-tidy and clang-format
winget install Cppcheck.Cppcheck      # static analysis
```

### Step 7: Configure and build (once the project is scaffolded)
Run from a **x64 Native Tools Command Prompt for VS 2022**, or from PowerShell after loading the VS environment:
```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build
ctest --test-dir build --output-on-failure
```

### Step 8: Verify the toolchain with a smoke test
Create a throwaway folder with a one-file Qt app and a `CMakeLists.txt` that calls `find_package(Qt6 REQUIRED COMPONENTS Widgets)`. If it configures, builds, and shows a window, the toolchain is ready.

### Step 9 (speech phase only): whisper.cpp
1. Add whisper.cpp as a git submodule under `third_party/whisper.cpp` and include it with `add_subdirectory`.
2. Download a model (e.g. `ggml-base.en.bin` or `ggml-small.bin`) from the whisper.cpp model repository. Do **not** commit model files. Download them on first use into `%LOCALAPPDATA%\Scribe\models`.
3. Optional GPU acceleration: build with Vulkan (works on most GPUs) or CUDA (NVIDIA only).

## 4. Planned Repository Layout

```
Scribe/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── .clang-format
├── .clang-tidy
├── .github/workflows/ci.yml
├── docs/                 project documents
├── src/
│   ├── app/              main.cpp, MainWindow, single-instance logic
│   ├── core/             Document, IDocumentBackend, encoding, line endings,
│   │                     CommandRegistry, SettingsManager, SessionManager
│   ├── editor/           EditorWidget, LineNumberArea, FindReplace, StatusBar
│   ├── platform/win/     DWM dark mode, jump list, shell integration, file sharing
│   ├── plugins/          IPlugin, IEditorHost, PluginManager
│   └── speech/           ISpeechEngine, WhisperEngine, audio capture
├── plugins/              sample plugins (JSON formatter, Base64, word count)
├── tests/                Qt Test and GoogleTest suites
├── resources/            icons, themes (JSON/QSS), default snippets
├── installer/            Inno Setup script
└── third_party/          vendored dependencies (whisper.cpp)
```

## 5. Coding Standards

- C++20, no raw `new`/`delete` outside Qt parent-child ownership.
- Keep logic in `core/` free of widget code so it can be unit tested.
- Every editor action is a **command** registered in `CommandRegistry` (menus, shortcuts, macros, command palette, and plugins all use it).
- Run `clang-format` before committing. CI fails on `clang-tidy` warnings configured as errors.
- Public interfaces (`IDocumentBackend`, `IPlugin`, `IEditorHost`, `ISpeechEngine`) are small and documented.

## 6. Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| `Could not find Qt6` | `CMAKE_PREFIX_PATH` is not set to the MSVC kit folder |
| App fails to start with missing `Qt6Core.dll` | Run `windeployqt` on the exe, or add Qt `bin` to `PATH` |
| Linker errors mixing Debug/Release | Use separate build directories per configuration |
| vcpkg packages not found | Pass `CMAKE_TOOLCHAIN_FILE` and make sure `vcpkg.json` is in the source root |
| Blurry UI on high-DPI screens | Do not disable Qt's high-DPI scaling, and check the manifest on the exe |
