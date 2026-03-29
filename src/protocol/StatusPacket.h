#pragma once
#include "Protocol.h"
#include <QByteArray>
#include <array>
#include <cstdint>

// Decoded representation of the 35-byte STATUS packet from the amplifier.
struct StatusPacket {
    // ── [04] STATUS_CODE ─────────────────────────────────────────────────────
    bool startupOperate = false;    // true = starts in OPERATE, false = STANDBY

    // ── [05] FLAGS ───────────────────────────────────────────────────────────
    bool tuning      = false;       // auto-tune in progress
    bool operate     = false;       // true = OPERATE, false = STANDBY
    bool txActive    = false;       // transceiver in TX
    bool alarmActive = false;       // alarm in progress
    bool fullMode    = false;       // true = FULL power, false = HALF
    bool contest     = false;
    bool beep        = false;
    bool tempCelsius = true;        // true = °C, false = °F

    // ── [06] DISPLAY_CTX ─────────────────────────────────────────────────────
    DisplayContext displayCtx = DisplayContext::Unknown;

    // ── [07–17] SETUP_0..SETUP_10 ────────────────────────────────────────────
    std::array<uint8_t, 11> setup{};

    // ── [18] BAND | INPUT ────────────────────────────────────────────────────
    Band    band  = Band::Unknown;
    uint8_t input = 0;              // 0 = IN1, 1 = IN2

    // ── [19] SUB_BAND ────────────────────────────────────────────────────────
    uint8_t subBand = 0;

    // ── [20–21] FREQ_LO / FREQ_HI ────────────────────────────────────────────
    uint16_t frequencyKHz = 0;      // 0..55000 kHz

    // ── [22] ANTENNA | CAT ───────────────────────────────────────────────────
    CatInterface catInterface = CatInterface::NONE;
    Antenna      antenna      = Antenna::None;

    // ── [23–24] SWR_LO/GAIN_LO | SWR_HI/GAIN_HI ─────────────────────────────
    // In STANDBY: SWR × 100  (0 = no signal, 9999 = ∞)
    // In OPERATE: PA gain × 10 dB
    uint16_t swrOrGainRaw = 0;

    // ── [25] TEMPERATURE ─────────────────────────────────────────────────────
    uint8_t temperature = 0;        // °C or °F per tempCelsius flag

    // ── [26–27] PA_OUT_LO / PA_OUT_HI ────────────────────────────────────────
    // Wpep × 10 (STANDBY = exciter power, OPERATE = PA output)
    uint16_t paOutputRaw = 0;

    // ── [28–29] PR_LO / PR_HI ────────────────────────────────────────────────
    uint16_t reversePowerRaw = 0;   // Wpep × 10

    // ── [30–31] VA_LO / VA_HI ────────────────────────────────────────────────
    uint16_t supplyVoltageRaw = 0;  // Volts × 10

    // ── [32–33] IA_LO / IA_HI ────────────────────────────────────────────────
    uint16_t supplyCurrentRaw = 0;  // Amps × 10

    // ── Convenience accessors ─────────────────────────────────────────────────
    double frequencyMHz()    const { return frequencyKHz / 1000.0; }
    double paOutputWpep()    const { return paOutputRaw / 10.0; }
    double reversePowerWpep()const { return reversePowerRaw / 10.0; }
    double supplyVoltage()   const { return supplyVoltageRaw / 10.0; }
    double supplyCurrent()   const { return supplyCurrentRaw / 10.0; }

    double swr() const {
        if (!operate) return swrOrGainRaw / 100.0;
        return 0.0;
    }
    double gainDb() const {
        if (operate) return swrOrGainRaw / 10.0;
        return 0.0;
    }

    // ── Decoder ───────────────────────────────────────────────────────────────
    // Expects exactly STATUS_TOTAL_BYTES bytes starting from the first 0xAA SYN.
    // Returns false if the checksum fails or the packet is malformed.
    static bool decode(const QByteArray& raw, StatusPacket& out);

    // Returns a human-readable warning string for WARNING_STATUS display contexts,
    // or an empty string if the context is not a warning/alarm.
    static const char* warningText(DisplayContext ctx);
};
