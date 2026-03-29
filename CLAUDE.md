# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**SPE 1K-FA Control** — Cross-platform (macOS, Linux, Windows) Qt6/C++20 application to remotely control the SPE Expert 1K-FA linear amplifier. Replicates all functions of the original Windows-only `EXPERT_Console.exe`, with added TCP connection support alongside the original RS-232 serial interface.

## Build

```bash
# Configure (Qt6 must be in PATH or CMAKE_PREFIX_PATH set)
cmake -B build -DCMAKE_BUILD_TYPE=Release
# or for debug:
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# macOS: run the app bundle
open build/SPE1KFAControl.app
# Linux/Windows: run the binary directly
./build/SPE1KFAControl
```

If Qt6 is not on PATH, prefix with: `cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/macos`

## Architecture

```
src/
├── main.cpp
├── protocol/
│   ├── Protocol.h          — all constants, enums (Band, CatInterface, Antenna, DisplayContext)
│   ├── StatusPacket.h/cpp  — 35-byte STATUS packet decoder; all telemetry fields
│   └── CommandBuilder.h    — header-only; builds binary command QByteArrays
├── connection/
│   ├── AbstractConnection.h — QObject interface (signals: dataReceived, connectionOpened, etc.)
│   ├── SerialConnection.*   — QSerialPort wrapper; 9600 8N1; supports DTR power control
│   └── TcpConnection.*      — QTcpSocket wrapper; IP:port; no DTR
├── controller/
│   └── AmplifierController.* — owns the connection; drives protocol state machine parser;
│                               sends RCU_ON on connect; emits statusUpdated(StatusPacket)
├── logging/
│   └── TelemetryLogger.*    — appends CSV rows on each statusUpdated signal
└── ui/
    ├── MainWindow.*         — main window; dark theme; connects all signals/slots
    ├── ConnectionDialog.*   — tabbed Serial/TCP connection dialog; persists settings via QSettings
    ├── AlarmHistoryDialog.* — displays alarm history from STATUS SETUP bytes
    └── widgets/
        ├── MeterWidget.*    — custom-painted horizontal bar meter with peak hold
        └── LedIndicator.*   — small circular LED with label
```

## Protocol Summary (SPE Expert Protocol Rev 2.0)

**Transport:** RS-232 9600 8N1 (no parity, no flow control), or TCP via serial-to-TCP adapter.

**Framing:**
- PC → Amp: `0x55 0x55 0x55 <CNT> <DATA...> <CHECKSUM>`
- Amp → PC: `0xAA 0xAA 0xAA <CNT> <DATA...> <CHECKSUM>`
- Checksum = sum of data bytes mod 256

**Commands (PC → Amp):**
- `CMD_KEY_ON (0x10) + key_code` — emulate front panel key
- `CMD_RCU_ON (0x80)` — start streaming STATUS at 5–8 Hz
- `CMD_RCU_OFF (0x81)` — stop streaming / poll for one STATUS
- `CMD_CAT_232 (0x82) + freq_lo + freq_hi` — set frequency in kHz

**Responses (Amp → PC):**
- ACK `(0x06)` — command accepted (when RCU_ON)
- NAK `(0x15)` — checksum/format error
- UNK `(0xFF)` — unknown command
- STATUS — 35-byte packet with full telemetry (CNT = 0x1E)

**STATUS packet key fields (zero-indexed from first SYN byte):**
- `[5]` FLAGS: TUNE, OP/STBY, TX, ALARM, MODE(FULL/HALF), CONTEST, BEEP, T_SCALE
- `[6]` DISPLAY_CTX: what the amp LCD is currently showing (drives SETUP bytes interpretation)
- `[18]` BAND(7:4) | INPUT(3:0)
- `[20:21]` Frequency in kHz (little-endian)
- `[22]` CAT(7:4) | ANTENNA(3:0)
- `[23:24]` SWR×100 (STANDBY) or PA gain×10 dB (OPERATE)
- `[25]` Temperature (°C or °F per FLAGS.T_SCALE)
- `[26:27]` PA output Wpep×10
- `[28:29]` Reverse power Wpep×10
- `[30:31]` Supply voltage V×10
- `[32:33]` Supply current A×10
- `[34]` Checksum (bytes [4]..[33] mod 256)

**DTR line (serial only):** assert high = power on amp; de-assert = power off. Not available over TCP.

## Key Design Decisions

- **RCU_ON mode** is used exclusively — amp streams STATUS automatically; no polling needed.
- **Parser** in `AmplifierController` is a byte-by-byte state machine (not packet-boundary aware) to handle OS serial buffering.
- **Connection abstraction** allows `SerialConnection` and `TcpConnection` to be swapped without changing any higher-layer code.
- **CommandBuilder** is a header-only namespace of inline functions — no .cpp needed.
- **Logging** is opt-in CSV; one row per STATUS packet; file appended (safe to restart).
- **QSettings** stores last-used connection parameters (IniFormat).

## Qt Modules Required

`Qt6::Core  Qt6::Widgets  Qt6::SerialPort  Qt6::Network`

## License

GPLv3
