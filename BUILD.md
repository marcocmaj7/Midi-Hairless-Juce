# Build Instructions

## Quick Start

### Windows (PowerShell)

```powershell
# Navigate to project directory
cd Midi-Hairless-JUCE

# Create build directory
mkdir build
cd build

# Option 1: Visual Studio (Recommended)
cmake .. -G "Visual Studio 17 2022" -A x64

# Option 2: MinGW
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run
.\HairlessMidiSerial_artefacts\Release\HairlessMidiSerial.exe
```

### macOS

```bash
cd Midi-Hairless-JUCE
mkdir build && cd build

# Generate Xcode project
cmake .. -G Xcode

# Build
cmake --build . --config Release

# Run
open HairlessMidiSerial_artefacts/Release/HairlessMidiSerial.app
```

### Linux

```bash
cd Midi-Hairless-JUCE
mkdir build && cd build

# Install dependencies first (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libasound2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxcursor-dev \
    libgl1-mesa-dev \
    libudev-dev

# Generate Makefiles
cmake ..

# Build (using all CPU cores)
make -j$(nproc)

# Run
./HairlessMidiSerial_artefacts/HairlessMidiSerial
```

## CMake Options

### Use Local JUCE Installation

If you have JUCE installed locally:

```bash
cmake .. -DJUCE_DIR=/path/to/JUCE
```

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### Custom Install Prefix

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . --config Release
cmake --install .
```

## IDE Integration

### Visual Studio (Windows)

1. Open CMake project directly in Visual Studio 2019+
2. Or generate .sln file:
   ```powershell
   cmake .. -G "Visual Studio 17 2022"
   start HairlessMidiSerial.sln
   ```

### Xcode (macOS)

```bash
cmake .. -G Xcode
open HairlessMidiSerial.xcodeproj
```

### CLion / VS Code

Simply open the `Midi-Hairless-JUCE` folder - CMake will be detected automatically.

## Troubleshooting

### "JUCE not found"

The CMakeLists.txt uses FetchContent to download JUCE automatically. If this fails:

1. Check your internet connection
2. Or download JUCE manually:
   ```bash
   git clone https://github.com/juce-framework/JUCE.git
   cmake .. -DJUCE_DIR=path/to/JUCE
   ```

### Windows: Missing Visual Studio

Install Visual Studio 2019 or later with "Desktop development with C++" workload.

Or use MinGW:
```powershell
# Install MinGW via MSYS2 or standalone
cmake .. -G "MinGW Makefiles"
```

### Linux: Missing headers

```bash
# Ubuntu/Debian
sudo apt-get install libasound2-dev libx11-dev libfreetype6-dev

# Fedora/RHEL
sudo dnf install alsa-lib-devel libX11-devel freetype-devel

# Arch Linux
sudo pacman -S alsa-lib libx11 freetype2
```

### macOS: Xcode command line tools

```bash
xcode-select --install
```

## Build Artifacts

After successful build, you'll find:

**Windows:**
```
build/HairlessMidiSerial_artefacts/Release/HairlessMidiSerial.exe
```

**macOS:**
```
build/HairlessMidiSerial_artefacts/Release/HairlessMidiSerial.app
```

**Linux:**
```
build/HairlessMidiSerial_artefacts/HairlessMidiSerial
```

## Clean Build

```bash
# Remove build directory
rm -rf build  # Linux/macOS
rmdir /s build  # Windows

# Start fresh
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
