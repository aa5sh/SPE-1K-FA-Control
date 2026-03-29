#pragma once
#include "Protocol.h"
#include <QByteArray>
#include <cstdint>

// Builds binary command packets for transmission to the amplifier.
// All packets follow: [0x55, 0x55, 0x55, CNT, DATA..., CHECKSUM]
namespace CommandBuilder {

namespace detail {
    inline QByteArray build(std::initializer_list<uint8_t> data)
    {
        QByteArray pkt;
        pkt.reserve(static_cast<qsizetype>(5 + data.size()));
        pkt.append(static_cast<char>(PC_SYN));
        pkt.append(static_cast<char>(PC_SYN));
        pkt.append(static_cast<char>(PC_SYN));
        pkt.append(static_cast<char>(data.size()));
        uint8_t chk = 0;
        for (uint8_t b : data) {
            pkt.append(static_cast<char>(b));
            chk += b;
        }
        pkt.append(static_cast<char>(chk));
        return pkt;
    }
}

// Enable remote console update (amp streams STATUS at 5-8 Hz)
inline QByteArray rcuOn()  { return detail::build({CMD_RCU_ON});  }

// Disable RCU streaming; also acts as a status poll (returns STATUS packet)
inline QByteArray rcuOff() { return detail::build({CMD_RCU_OFF}); }

// Emulate a front panel key press
inline QByteArray keyPress(uint8_t keyCode) {
    return detail::build({CMD_KEY_ON, keyCode});
}

// Send frequency for CAT-driven band switching (freq in kHz)
inline QByteArray cat232(uint16_t freqKHz) {
    return detail::build({
        CMD_CAT_232,
        static_cast<uint8_t>(freqKHz & 0xFF),
        static_cast<uint8_t>((freqKHz >> 8) & 0xFF)
    });
}

// Convenience wrappers for every front-panel key
inline QByteArray keyLMinus()    { return keyPress(KEY_L_MINUS);    }
inline QByteArray keyLPlus()     { return keyPress(KEY_L_PLUS);     }
inline QByteArray keyCMinus()    { return keyPress(KEY_C_MINUS);    }
inline QByteArray keyCPlus()     { return keyPress(KEY_C_PLUS);     }
inline QByteArray keyTune()      { return keyPress(KEY_TUNE);       }
inline QByteArray keyIn()        { return keyPress(KEY_IN);         }
inline QByteArray keyBandMinus() { return keyPress(KEY_BAND_MINUS); }
inline QByteArray keyBandPlus()  { return keyPress(KEY_BAND_PLUS);  }
inline QByteArray keyAnt()       { return keyPress(KEY_ANT);        }
inline QByteArray keyCat()       { return keyPress(KEY_CAT);        }
inline QByteArray keyLeft()      { return keyPress(KEY_LEFT);       }
inline QByteArray keyRight()     { return keyPress(KEY_RIGHT);      }
inline QByteArray keySet()       { return keyPress(KEY_SET);        }
inline QByteArray keyOff()       { return keyPress(KEY_OFF);        }
inline QByteArray keyMode()      { return keyPress(KEY_MODE);       }
inline QByteArray keyDisplay()   { return keyPress(KEY_DISPLAY);    }
inline QByteArray keyOperate()   { return keyPress(KEY_OPERATE);    }

} // namespace CommandBuilder
