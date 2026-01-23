//
// Created by adama on 1/23/26.
//
#pragma once
#define MAX_BITSTREAM_SIZE 3
#include <cmath>
#include "types.hpp"
#include "pico/stdlib.h"

namespace DCC::Framing {
    inline uint32_t fillMSB(uint32_t num, uint8_t x) {
        if (x  >= 32) {
            return ~0UL;
        }
        if (x <= 0) {
            return num;
        }

        const uint32_t mask = ~(~0UL >> x);

        return num | mask;
    }

    inline void setBitAtIndex(uint32_t* arr, uint32_t bit, bool value) {
        if (value) {
            arr[bit / 32] |= (1UL << (bit % 32));
        }
    }

    inline void clearBitAtIndex(uint32_t* arr, uint32_t bit) {
        arr[bit / 32] &= ~(1UL << (bit % 32));
    }

    inline uint32_t insertByte(uint32_t num, uint8_t x, uint8_t idx) {
        if (idx > 31) {
            return num;
        }

        num &= ~(0xff << idx);
        uint32_t mask = x << idx;

        return num | mask;
    }

    struct BitstreamPacket {
        uint32_t buffer[MAX_BITSTREAM_SIZE] = {};
        uint8_t bitsUtilized = 0;
        uint8_t bytesFullyUtilized = 0;

        BitstreamPacket() = default;

        explicit BitstreamPacket(Packet* packet) {
            packetToBitstreamInternal(packet);
        }

        void reinit(Packet* packet) {
            packetToBitstreamInternal(packet);
        }

        private:
        void packetToBitstreamInternal(Packet* packet) {
            // fill with preamble bits
            uint8_t blTemp = MAX_BITSTREAM_SIZE * 32;
            buffer[2] = fillMSB(0, 14);
            blTemp -= 14;

            for (size_t i = 0; i < packet->length; i++) {
                // clear start bit
                clearBitAtIndex(buffer, blTemp--);

                for (int8_t j = 7; j >= 0; j--) {
                    setBitAtIndex(buffer, blTemp--, (packet->buffer[i] >> j) & 1);
                }
            }

            // set packet end bit
            setBitAtIndex(buffer, blTemp--, true);

            // calculate real packet length (bits)
            bitsUtilized = abs(blTemp - (MAX_BITSTREAM_SIZE * 32));
            bytesFullyUtilized = bitsUtilized / 32;

            // reverse list order from lsb first to msb first
            const uint32_t tmp = buffer[0];
            buffer[0] = buffer[2];
            buffer[2] = tmp;
        }
    };

}
