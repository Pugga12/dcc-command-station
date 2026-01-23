//
// Created by adama on 1/22/26.
//
#pragma once
#define MAX_PACKET_SIZE 6
#include <pico/stdlib.h>

namespace DCC {
    struct Packet {
        uint8_t buffer[MAX_PACKET_SIZE] = {};
        size_t length = 0;
    };
}
