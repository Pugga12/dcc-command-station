//
// Created by adama on 1/24/26.
//
#pragma once
#include <cstdint>

namespace DCC {
    inline size_t writeAddress(uint16_t address, uint8_t* buf) {
        if (address > 127) {
            buf[0] = static_cast<uint8_t>((address >> 8) & 0x3F) | 0xC0;
            buf[1] = static_cast<uint8_t>(address & 0xFF);
            return 2;
        } else {
            buf[0] = static_cast<uint8_t>(address) & 0x7F;
            return 1;
        }
    }
}
