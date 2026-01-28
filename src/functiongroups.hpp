//
// Created by adama on 1/24/26.
//
#pragma once
#include <cstdint>

#include "helpers.hpp"
#include "types.hpp"

namespace DCC::Packets {
    enum F2NybbleSelect : uint8_t {
        LOW,
        HIGH
    };

    struct FunctionGroupBuilder {
        static size_t buildFG1(const uint8_t f0_f4State, const uint16_t locoAddress, Packet* pkt) {
            size_t bytesWritten = writeAddress(locoAddress, pkt->buffer);
            pkt->buffer[bytesWritten++] = 0b10000000 | (f0_f4State & 0x1f);

            uint8_t checksum = 0;
            for (size_t i = 0; i < bytesWritten; i++) {
                checksum ^= pkt->buffer[i];
            }
            pkt->buffer[bytesWritten++] = checksum;
            pkt->length = bytesWritten;
            return bytesWritten;
        }

        static size_t buildFG2(const uint8_t f5_f12State, const uint16_t locoAddress, const F2NybbleSelect nybbleSelect, Packet* pkt) {
            size_t bytesWritten = writeAddress(locoAddress, pkt->buffer);
            uint8_t dataByte = 0b10100000;
            if (nybbleSelect == HIGH) {
                dataByte |=  (f5_f12State & 0x0f);
            } else {
                dataByte |= ( 0b00010000 | (f5_f12State >> 4));
            }

            pkt->buffer[bytesWritten++] = dataByte;
            uint8_t checksum = 0;
            for (size_t i = 0; i < bytesWritten; i++) {
                checksum ^= pkt->buffer[i];
            }
            pkt->buffer[bytesWritten++] = checksum;
            pkt->length = bytesWritten;
            return bytesWritten;
        }

        static size_t buildFG3(const uint8_t f13_f20State, const uint16_t locoAddress, Packet* pkt) {
            size_t bytesWritten = writeAddress(locoAddress, pkt->buffer);
            pkt->buffer[bytesWritten++] = 0b11011110;
            pkt->buffer[bytesWritten++] = f13_f20State;
            uint8_t checksum = 0;
            for (size_t i = 0; i < bytesWritten; i++) {
                checksum ^= pkt->buffer[i];
            }
            pkt->buffer[bytesWritten++] = checksum;
            pkt->length = bytesWritten;
            return bytesWritten;
        }
    };
}
