#pragma once
#include <cstdint>

// ── Framing ──────────────────────────────────────────────────────────────────
inline constexpr uint8_t PC_SYN  = 0x55;   // PC → Amp sync byte
inline constexpr uint8_t AMP_SYN = 0xAA;   // Amp → PC sync byte

// ── Response codes (single data byte from amp) ───────────────────────────────
inline constexpr uint8_t RESP_ACK = 0x06;
inline constexpr uint8_t RESP_NAK = 0x15;
inline constexpr uint8_t RESP_UNK = 0xFF;

// ── STATUS packet ────────────────────────────────────────────────────────────
inline constexpr uint8_t STATUS_CNT         = 0x1E;  // 30 data bytes
inline constexpr int     STATUS_TOTAL_BYTES = 35;    // 3 SYN + 1 CNT + 30 data + 1 chk

inline constexpr uint8_t STATUS_CODE_STBY   = 0xA0;
inline constexpr uint8_t STATUS_CODE_OPERATE = 0xA1;

// ── Command opcodes ───────────────────────────────────────────────────────────
inline constexpr uint8_t CMD_KEY_ON  = 0x10;
inline constexpr uint8_t CMD_RCU_ON  = 0x80;
inline constexpr uint8_t CMD_RCU_OFF = 0x81;
inline constexpr uint8_t CMD_CAT_232 = 0x82;

// ── Key codes (DATA byte for CMD_KEY_ON) ─────────────────────────────────────
// Left panel
inline constexpr uint8_t KEY_L_MINUS = 0x30;
inline constexpr uint8_t KEY_L_PLUS  = 0x31;
inline constexpr uint8_t KEY_C_MINUS = 0x32;
inline constexpr uint8_t KEY_C_PLUS  = 0x33;
inline constexpr uint8_t KEY_TUNE    = 0x34;
// Center panel
inline constexpr uint8_t KEY_IN        = 0x28;
inline constexpr uint8_t KEY_BAND_MINUS = 0x29;
inline constexpr uint8_t KEY_BAND_PLUS  = 0x2A;
inline constexpr uint8_t KEY_ANT        = 0x2B;
inline constexpr uint8_t KEY_CAT        = 0x2C;
inline constexpr uint8_t KEY_LEFT       = 0x2D;
inline constexpr uint8_t KEY_RIGHT      = 0x2E;
inline constexpr uint8_t KEY_SET        = 0x2F;
// Right panel
inline constexpr uint8_t KEY_OFF     = 0x18;
inline constexpr uint8_t KEY_MODE    = 0x1A;
inline constexpr uint8_t KEY_DISPLAY = 0x1B;
inline constexpr uint8_t KEY_OPERATE = 0x1C;

// ── FLAGS byte bit positions ──────────────────────────────────────────────────
inline constexpr uint8_t FLAG_TUNE    = 0x01;  // bit 0
inline constexpr uint8_t FLAG_OPSTBY  = 0x02;  // bit 1 — 1=OPERATE
inline constexpr uint8_t FLAG_TX      = 0x04;  // bit 2
inline constexpr uint8_t FLAG_ALARM   = 0x08;  // bit 3
inline constexpr uint8_t FLAG_MODE    = 0x10;  // bit 4 — 1=FULL, 0=HALF
inline constexpr uint8_t FLAG_CONTEST = 0x20;  // bit 5
inline constexpr uint8_t FLAG_BEEP    = 0x40;  // bit 6
inline constexpr uint8_t FLAG_TSCALE  = 0x80;  // bit 7 — 1=Celsius

// ── STATUS_CODE bit ───────────────────────────────────────────────────────────
inline constexpr uint8_t STATUS_STARTUP_BIT = 0x01;  // bit 0

// ── DISPLAY_CTX values ───────────────────────────────────────────────────────
enum class DisplayContext : uint8_t {
    Logo           = 0x00,
    PaOutIpa       = 0x01,
    PwRevVpa       = 0x02,
    CatInfo        = 0x03,
    // 0x04 = debug, ignore
    DataStored     = 0x05,
    SetupOptions   = 0x06,
    SetAntenna     = 0x07,
    SetCat         = 0x08,
    SetYaesu       = 0x09,
    SetIcom        = 0x0A,
    SetTenTec      = 0x0B,
    SetBaudrate    = 0x0C,
    ManualTune     = 0x0D,
    Backlight      = 0x0E,
    // 0x0F, 0x10 = debug, ignore
    WarnVoltLowHalf     = 0x11,
    WarnVoltLowFull     = 0x12,
    WarnVoltHighHalf    = 0x13,
    WarnVoltHighFull    = 0x14,
    WarnCurrentHighHalf = 0x15,
    WarnCurrentHighFull = 0x16,
    WarnTempHigh        = 0x17,
    WarnInputPowerHigh  = 0x18,
    // 0x19, 0x1A = debug, ignore
    WarnReversePower    = 0x1B,
    WarnPaProtection    = 0x1C,
    AlarmHistory        = 0x1D,
    Shutdown            = 0x1E,
    Unknown             = 0xFF,
};

// ── Band codes (BAND field, bits 7-4 of byte [18]) ───────────────────────────
enum class Band : uint8_t {
    M160 = 0, M80, M40, M30, M20, M17, M15, M12, M10, M6, Unknown = 0xFF
};

inline const char* bandName(Band b) {
    switch (b) {
        case Band::M160: return "160m";
        case Band::M80:  return "80m";
        case Band::M40:  return "40m";
        case Band::M30:  return "30m";
        case Band::M20:  return "20m";
        case Band::M17:  return "17m";
        case Band::M15:  return "15m";
        case Band::M12:  return "12m";
        case Band::M10:  return "10m";
        case Band::M6:   return "6m";
        default:         return "---";
    }
}

// ── CAT interface codes ───────────────────────────────────────────────────────
enum class CatInterface : uint8_t {
    SPE = 0, ICOM, KENWOOD, YAESU, TEN_TEC, FLEX_RADIO, RS232, NONE
};

inline const char* catName(CatInterface c) {
    switch (c) {
        case CatInterface::SPE:        return "SPE";
        case CatInterface::ICOM:       return "ICOM";
        case CatInterface::KENWOOD:    return "KENWOOD";
        case CatInterface::YAESU:      return "YAESU";
        case CatInterface::TEN_TEC:    return "TEN-TEC";
        case CatInterface::FLEX_RADIO: return "FLEX";
        case CatInterface::RS232:      return "RS-232";
        case CatInterface::NONE:       return "NONE";
        default:                       return "---";
    }
}

// ── Antenna codes ─────────────────────────────────────────────────────────────
enum class Antenna : uint8_t {
    Ant1 = 0, Ant2, Ant3, Ant4, None = 4
};

inline const char* antennaName(Antenna a) {
    switch (a) {
        case Antenna::Ant1: return "ANT 1";
        case Antenna::Ant2: return "ANT 2";
        case Antenna::Ant3: return "ANT 3";
        case Antenna::Ant4: return "ANT 4";
        case Antenna::None: return "NONE";
        default:            return "---";
    }
}

// ── SWR special values ────────────────────────────────────────────────────────
inline constexpr uint16_t SWR_NO_SIGNAL = 0;
inline constexpr uint16_t SWR_INFINITY  = 9999;
