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
#define SYSTEM_LOCOS_SUPPORTED 16
#define CAB_INACTIVE_TIMEOUT pdMS_TO_TICKS(5000)
#define MAX_CABS 31

namespace DCC::Registry {
    enum SlotStatus : uint8_t {
        FREE = 0,
        IN_USE = 1,
        TIMEOUT = 2,
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
        SlotStatus status = FREE;
        TickType_t lastTick = 0;

        uint8_t validityMask = 0;
        uint8_t dirtyMask = 0;
        Packets::Speed::SpeedState speed;
        uint32_t functionMask = 0;

        bool isExtendedAddress() {
            return id > 127;
        }
    };

    class LocoRegistry {
        LocoSlot slots[SYSTEM_LOCOS_SUPPORTED] = {};
        LocoSlot* slotCacheByCab[MAX_CABS] = {};
        uint32_t freeSlots = SYSTEM_LOCOS_SUPPORTED;

        void releaseSlotInternal(LocoSlot* slot) {
            slot->status = FREE;
            slot->id = 0;
            slot->assignedCab = 0;
            slot->lastTick = 0;
            slot->validityMask = 0;
            freeSlots++;
        }

        LocoSlot* getSlot(uint16_t locoId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            for (auto& slot : slots) {
                if (slot.id == locoId) {
                    return &slot;
                    xSemaphoreGive(semaphore);
                }
            }
            xSemaphoreGive(semaphore);
            return nullptr;
        }

        LocoSlot* getSlot(uint8_t cabId, bool useCache) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            if (useCache && slotCacheByCab[cabId] != nullptr) {
                xSemaphoreGive(semaphore);
                return slotCacheByCab[cabId];
            } else {
                for (auto& slot : slots) {
                    if (slot.id == cabId) {
                        xSemaphoreGive(semaphore);
                        return &slot;
                    }
                }
            }

            xSemaphoreGive(semaphore);
            return nullptr;
        }

        public:
        SemaphoreHandle_t semaphore;
        LocoRegistry() {
            semaphore = xSemaphoreCreateBinary();
            configASSERT(semaphore);
        }

        LocoSlot* acquireSlot(uint16_t address, uint8_t cabId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            if (freeSlots == 0) {
                xSemaphoreGive(semaphore);
                return nullptr;
            }

            for (auto& slot : slots) {
                if (slot.status == FREE) {
                    freeSlots--;
                    slot.id = address;
                    slot.assignedCab = cabId;
                    slot.status = IN_USE;
                    slot.lastTick = xTaskGetTickCount();
                    slot.validityMask = 0;
                    slotCacheByCab[cabId] = &slot;

                    xSemaphoreGive(semaphore);
                    return &slot;
                }
            }

            xSemaphoreGive(semaphore);
            return nullptr;
        }

        void requestSlotRelease(uint16_t address) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            for (auto& slot : slots) {
                if (slot.status != FREE && slot.id == address) {
                    releaseSlotInternal(&slot);
                    slotCacheByCab[slot.id] = nullptr;
                }
            }
            xSemaphoreGive(semaphore);
        }

        void requestSlotReleaseFromCache(uint8_t cabId) {
            xSemaphoreTake(semaphore, portMAX_DELAY);
            if (slotCacheByCab[cabId] != nullptr) {
                releaseSlotInternal(slotCacheByCab[cabId]);
            }
            xSemaphoreGive(semaphore);
        }

        void forEachActiveSlot(std::function<void(LocoSlot& slot)> func) {
            for (auto& slot : slots) {
                if (slot.status == FREE && slot.validityMask != 0) {
                    func(slot);
                }
            }
        }

        bool setSpeedMode()
    };
}
