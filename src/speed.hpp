//
// Created by adama on 1/24/26.
//
#pragma once
#include <cstdint>
#include "types.hpp"

namespace DCC::Packets::Speed {
    enum SpeedMode : uint8_t {
        DCC14,
        DCC28,
        DCC128
    };

    enum Direction : uint8_t {
        FORWARD,
        BACKWARD
    };

    struct SpeedState {
        uint8_t speed = 0;
        Direction direction = FORWARD;
        SpeedMode mode = DCC128;
    };

    struct SpeedPacketAssembler {
        Packet build(uint16_t locoAddress, SpeedState state) {
            switch (state.mode) {
                case DCC14:
                    buildPacketImpl<DCC14>(locoAddress, state);
                    break;
                case DCC28:
                    buildPacketImpl<DCC28>(locoAddress, state);
                    break;
                case DCC128:
                    buildPacketImpl<DCC128>(locoAddress, state);
            }
        }

        private:
        template<SpeedMode mode>
        Packet buildPacketImpl(uint16_t locoAddress, SpeedState state) {
            if constexpr (mode == DCC14) {

            }
        }
    };
}
