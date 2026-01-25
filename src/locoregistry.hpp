//
// Created by adama on 1/22/26.
//
#pragma once
#include "portmacro.h"
#include "projdefs.h"
#include "types.hpp"
#include "semphr.h"
#include "task.h"
#include <functional>
#include "speed.hpp"

#include "framing.hpp"
#include <cstdint>
#define SYSTEM_LOCOS_SUPPORTED 16
#define CAB_INACTIVE_TIMEOUT pdMS_TO_TICKS(5000)
#define MAX_CABS 31
#include "etl/map.h"

namespace DCC::Registry {
    enum SlotStatus : uint8_t {
        ACTIVE = 0,
        TIMEOUT = 1,
        MARKED_FOR_DELETION = 2
    };

    enum ValidityMask : uint8_t {
        SPEED_VALID = 1 << 0,
        FUNCTION_1_VALID = 1 << 1,
        FUNCTION_2_VALID = 1 << 2,
        FUNCTION_3_VALID = 1 << 3,
    };

    struct LocoSlot {
        uint16_t id = 0;
        uint8_t assignedCab = 0;
        SlotStatus status = ACTIVE;
        TickType_t lastTick = 0;

        uint8_t validityMask = 0;
        uint8_t dirtyMask = 0;
        Packets::Speed::SpeedState speed;
        uint8_t f0_f4State = 0;
        uint8_t f5_f12State = 0;
        uint8_t f13_f20State = 0;

        bool isExtendedAddress() {
            return id > 127;
        }
    };

    class LocoRegistry {
        etl::map<uint16_t, LocoSlot, SYSTEM_LOCOS_SUPPORTED> locoSlots;

    public:
        SemaphoreHandle_t semaphore;

        LocoRegistry() {
            semaphore = xSemaphoreCreateMutex();
            configASSERT(semaphore);
        }

        bool acquireSlot(uint16_t locoId, uint16_t cabId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);

            if (locoSlots.find(locoId) != locoSlots.end()) {
                xSemaphoreGive(semaphore);
                return false;
            }
            if (locoSlots.full()) {
                xSemaphoreGive(semaphore);
                return false;
            }

            LocoSlot slot;
            slot.assignedCab = cabId;
            slot.status = ACTIVE;
            slot.id = locoId;
            slot.lastTick = xTaskGetTickCount();
            locoSlots.insert({locoId, slot});
            xSemaphoreGive(semaphore);
            return true;
        }

        bool forceSlotDelete(uint16_t locoId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            bool status = locoSlots.erase(locoId);
            xSemaphoreGive(semaphore);
            return status;
        }

        bool requestSlotCleanup(uint16_t locoId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            auto slot = locoSlots.find(locoId);

            if (slot != locoSlots.end()) {
                slot->second.status = MARKED_FOR_DELETION;
                slot->second.lastTick = xTaskGetTickCount();
                xSemaphoreGive(semaphore);
                return true;
            }

            xSemaphoreGive(semaphore);
            return false;
        }

        bool setSpeedState(uint16_t locoId, Packets::Speed::SpeedState newState) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            auto slotPair = locoSlots.find(locoId);

            if (slotPair == locoSlots.end()) {
                xSemaphoreGive(semaphore);
                return false;
            }
            auto slot = slotPair->second;
            if (slot.status == TIMEOUT) slot.status = ACTIVE;
            newState.clamp();
            slot.speed = newState;
            slot.validityMask |= SPEED_VALID;
            slot.dirtyMask |= SPEED_VALID;
            slot.lastTick = xTaskGetTickCount();
            xSemaphoreGive(semaphore);

            return true;
        }

        bool setFunctionMasks(uint16_t locoId, uint8_t f0_f4State, uint8_t f5_f12State, uint8_t f13_f20State) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            auto slotPair = locoSlots.find(locoId);

            if (slotPair == locoSlots.end()) {
                xSemaphoreGive(semaphore);
                return false;
            }

            auto slot = slotPair->second;
            if (slot.status == TIMEOUT) slot.status = ACTIVE;
            if (slot.f0_f4State != f0_f4State) {
                slot.f0_f4State = f0_f4State & 0x1f;
                slot.dirtyMask |= FUNCTION_1_VALID;
            };
            if (slot.f5_f12State != f5_f12State) {
                slot.f5_f12State = f5_f12State;
                slot.dirtyMask |= FUNCTION_2_VALID;
            }
            if (slot.f13_f20State != f13_f20State) {
                slot.f13_f20State = f13_f20State;
                slot.dirtyMask |= FUNCTION_3_VALID;
            }
            slot.lastTick = xTaskGetTickCount();
            xSemaphoreGive(semaphore);
            return true;
        }

        bool keepAlive(uint16_t locoId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            auto slotPair = locoSlots.find(locoId);

            if (slotPair == locoSlots.end()) {
                xSemaphoreGive(semaphore);
                return false;
            }

            auto slot = slotPair->second;
            slot.lastTick = xTaskGetTickCount();
            slot.status = ACTIVE;
            xSemaphoreGive(semaphore);
            return true;
        }
    };
}
