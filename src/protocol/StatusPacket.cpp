#include "StatusPacket.h"

bool StatusPacket::decode(const QByteArray& raw, StatusPacket& out)
{
    // Validate size and framing
    if (raw.size() < STATUS_TOTAL_BYTES)
        return false;

    const auto* d = reinterpret_cast<const uint8_t*>(raw.constData());

    if (d[0] != AMP_SYN || d[1] != AMP_SYN || d[2] != AMP_SYN)
        return false;
    if (d[3] != STATUS_CNT)
        return false;

    // Verify checksum: sum of bytes [4]..[33] mod 256
    uint8_t chk = 0;
    for (int i = 4; i <= 33; ++i)
        chk += d[i];
    if (chk != d[34])
        return false;

    // [04] STATUS_CODE
    out.startupOperate = (d[4] & STATUS_STARTUP_BIT) != 0;

    // [05] FLAGS
    out.tuning      = (d[5] & FLAG_TUNE)    != 0;
    out.operate     = (d[5] & FLAG_OPSTBY)  != 0;
    out.txActive    = (d[5] & FLAG_TX)      != 0;
    out.alarmActive = (d[5] & FLAG_ALARM)   != 0;
    out.fullMode    = (d[5] & FLAG_MODE)    != 0;
    out.contest     = (d[5] & FLAG_CONTEST) != 0;
    out.beep        = (d[5] & FLAG_BEEP)    != 0;
    out.tempCelsius = (d[5] & FLAG_TSCALE)  != 0;

    // [06] DISPLAY_CTX — map to enum, unknown values become Unknown
    const uint8_t ctx = d[6];
    // Debug values: 0x04, 0x0F, 0x10, 0x19, 0x1A — treat as Unknown
    if (ctx == 0x04 || ctx == 0x0F || ctx == 0x10 || ctx == 0x19 || ctx == 0x1A)
        out.displayCtx = DisplayContext::Unknown;
    else
        out.displayCtx = static_cast<DisplayContext>(ctx);

    // [07–17] SETUP_0..SETUP_10
    for (int i = 0; i < 11; ++i)
        out.setup[static_cast<size_t>(i)] = d[7 + i];

    // [18] BAND | INPUT
    const uint8_t bi = d[18];
    const uint8_t bandNibble = (bi >> 4) & 0x0F;
    out.band  = (bandNibble <= 9) ? static_cast<Band>(bandNibble) : Band::Unknown;
    out.input = bi & 0x0F;

    // [19] SUB_BAND
    out.subBand = d[19] & 0x7F;

    // [20–21] FREQ
    out.frequencyKHz = static_cast<uint16_t>(d[20]) |
                       (static_cast<uint16_t>(d[21]) << 8);

    // [22] ANTENNA | CAT
    const uint8_t ac = d[22];
    const uint8_t catNibble = (ac >> 4) & 0x0F;
    const uint8_t antNibble =  ac       & 0x0F;
    out.catInterface = (catNibble <= 7) ? static_cast<CatInterface>(catNibble)
                                        : CatInterface::NONE;
    out.antenna      = (antNibble <= 4) ? static_cast<Antenna>(antNibble)
                                        : Antenna::None;

    // [23–24] SWR / GAIN
    out.swrOrGainRaw = static_cast<uint16_t>(d[23]) |
                       (static_cast<uint16_t>(d[24]) << 8);

    // [25] TEMPERATURE
    out.temperature = d[25];

    // [26–27] PA_OUT
    out.paOutputRaw = static_cast<uint16_t>(d[26]) |
                      (static_cast<uint16_t>(d[27]) << 8);

    // [28–29] PR (reverse power)
    out.reversePowerRaw = static_cast<uint16_t>(d[28]) |
                          (static_cast<uint16_t>(d[29]) << 8);

    // [30–31] VA (supply voltage)
    out.supplyVoltageRaw = static_cast<uint16_t>(d[30]) |
                           (static_cast<uint16_t>(d[31]) << 8);

    // [32–33] IA (supply current)
    out.supplyCurrentRaw = static_cast<uint16_t>(d[32]) |
                           (static_cast<uint16_t>(d[33]) << 8);

    return true;
}

const char* StatusPacket::warningText(DisplayContext ctx)
{
    switch (ctx) {
        case DisplayContext::WarnVoltLowHalf:     return "WARNING: Supply voltage low (<20V, HALF mode)";
        case DisplayContext::WarnVoltLowFull:     return "WARNING: Supply voltage low (<26V, FULL mode)";
        case DisplayContext::WarnVoltHighHalf:    return "WARNING: Supply voltage high (>50V, HALF mode)";
        case DisplayContext::WarnVoltHighFull:    return "WARNING: Supply voltage high (>50V, FULL mode)";
        case DisplayContext::WarnCurrentHighHalf: return "WARNING: Supply current high (>40A, HALF mode)";
        case DisplayContext::WarnCurrentHighFull: return "WARNING: Supply current high (>50A, FULL mode)";
        case DisplayContext::WarnTempHigh:        return "WARNING: Temperature high (>90°C)";
        case DisplayContext::WarnInputPowerHigh:  return "WARNING: Input power too high";
        case DisplayContext::WarnReversePower:    return "WARNING: Reverse power high (>300W PEP)";
        case DisplayContext::WarnPaProtection:    return "WARNING: PA protection activated";
        case DisplayContext::Shutdown:            return "SHUTDOWN in progress";
        default:                                  return "";
    }
}
