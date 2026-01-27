//
// Created by adama on 1/24/26.
//
#pragma once
#include <algorithm>
#include <cstdint>
#include "types.hpp"
#include "helpers.hpp"

namespace DCC::Packets {
    enum SpeedMode : uint8_t {
        DCC14,
        DCC28,
        DCC128
    };

    enum Direction : uint8_t {
        BACKWARD = 0,
        FORWARD = 1
    };

    struct SpeedState {
        int speed = 0;
        Direction direction = FORWARD;
        SpeedMode mode = DCC128;

        void clamp() {
            switch (mode) {
                case DCC14:
                    speed = std::clamp(speed, 0, 15);
                    break;
                case DCC28:
                    speed = std::clamp(speed, 0, 27);
                    break;
                case DCC128:
                    speed = std::clamp(speed, 0, 127);
            }
        }
    };

    struct SpeedPacketAssembler {
        static void build(uint16_t locoAddress, SpeedState state, Packet* pkt) {
            switch (state.mode) {
                case DCC14:
                    buildPacketImpl<DCC14>(locoAddress, state, pkt);
                    break;
                case DCC28:
                    buildPacketImpl<DCC28>(locoAddress, state, pkt);
                    break;
                case DCC128:
                    buildPacketImpl<DCC128>(locoAddress, state, pkt);
            }
        }

        private:
        template<SpeedMode mode>
        static size_t buildPacketImpl(const uint16_t locoAddress, SpeedState state, Packet* pkt) {
            size_t bytesWritten = 0;

            if constexpr (mode == DCC14) {
                bytesWritten = writeAddress(locoAddress, pkt->buffer);
                uint8_t dataByte = 0b01000000;
                if (state.direction == FORWARD) dataByte |= 0b00100000;
                dataByte |= (state.speed & 0x7f);
                pkt->buffer[bytesWritten++] = dataByte;
            } else if constexpr (mode == DCC28) {
                bytesWritten = writeAddress(locoAddress, pkt->buffer);
                uint8_t dataByte = 0b01000000;
                if (state.direction == FORWARD) dataByte |= 0b00100000;
                dataByte |= (state.speed & 0x1f);
                pkt->buffer[bytesWritten++] = dataByte;

                return bytesWritten;
            } else {
                bytesWritten = writeAddress(locoAddress, pkt->buffer);
                pkt->buffer[bytesWritten++] = 0b00111111;
                uint8_t dataByte = 0;
                if (state.direction == FORWARD) dataByte |= 0b10000000;
                dataByte |= (state.speed & 0x7f);
                pkt->buffer[bytesWritten++] = dataByte;
            }

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
