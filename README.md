# SPE 1K-FA Control

A cross-platform Qt6/C++20 desktop application for remotely controlling the **SPE Expert 1K-FA** linear amplifier. Replicates all functions of the original Windows-only `EXPERT_Console.exe`, with added TCP connection support alongside the original RS-232 serial interface.

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-blue)
![License](https://img.shields.io/badge/license-GPLv3-green)
![Qt](https://img.shields.io/badge/Qt-6.2%2B-brightgreen)

## Features

- Connect via **RS-232 serial** (9600 8N1) or **TCP** (serial-to-Ethernet adapter)
- Real-time telemetry: PA output, reverse power, supply voltage/current, temperature
- Bar meters with peak hold for PA Out, IPA, PW Rev, and VPA
- Full setup menu display: antenna assignments, CAT configuration, baud rate, backlight, manual tune
- **DTR power control** — power the amp on/off via RTS/DTR line (serial only)
- CSV telemetry logging
- Alarm history viewer
- Always-on-top window option
- Dark theme with Fusion style

## Screenshots

_Connect via the File menu or toolbar, then watch live telemetry stream in._

## Build from Source

### Prerequisites

| Platform | Requirement |
|----------|-------------|
| All | CMake ≥ 3.21, C++20 compiler, Qt 6.2+ (Core, Widgets, SerialPort, Network) |
| macOS | Xcode command-line tools, Homebrew (recommended) |
| Linux | GCC 11+ or Clang 13+, system Qt6 packages |
| Windows | MSVC 2022 or MinGW, Qt6 installer from qt.io |

---

### macOS (Homebrew)

```bash
# Install Qt6
brew install qt

# Clone and build
git clone https://github.com/aa5sh/spe-1k-fa-control.git
cd spe-1k-fa-control
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --config Release

# Run
open build/SPE1KFAControl.app
```

---

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y cmake build-essential \
    qt6-base-dev qt6-serialport-dev libqt6network6 \
    libgl1-mesa-dev

# Clone and build
git clone https://github.com/aa5sh/spe-1k-fa-control.git
cd spe-1k-fa-control
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run
./build/SPE1KFAControl
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtserialport-devel
```

**Serial port access (Linux):** Add your user to the `dialout` group, then log out and back in:
```bash
sudo usermod -a -G dialout $USER
```

---

### Windows (MSVC)

1. Install [Qt 6.x](https://www.qt.io/download-qt-installer) — select **MSVC 2022 64-bit**, **Qt Serial Port**, and **Qt Network** components
2. Install [CMake](https://cmake.org/download/) and [Visual Studio 2022](https://visualstudio.microsoft.com/) (Desktop C++ workload)

```bat
git clone https://github.com/aa5sh/spe-1k-fa-control.git
cd spe-1k-fa-control

cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
```

The executable will be at `build\Release\SPE1KFAControl.exe`.

To distribute, run `windeployqt` to bundle Qt DLLs:
```bat
"C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe" build\Release\SPE1KFAControl.exe
```

---

## Connection

| Type | Settings |
|------|----------|
| Serial | 9600 baud, 8N1, no flow control |
| TCP | IP address + port of your serial-to-Ethernet adapter |

The app sends `RCU_ON` on connect and retries every second until the amplifier responds with a STATUS packet (useful after DTR power-on, which takes 7–10 s for the amp to boot).

## Protocol

The app implements the **SPE Expert Remote Control Protocol Rev 2.0**. See `CLAUDE.md` for a full protocol summary including packet framing, command codes, and STATUS packet field layout.

## License

GPLv3 — see [LICENSE](LICENSE)
