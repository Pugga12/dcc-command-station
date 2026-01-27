#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "locoregistry.hpp"
#include "framing.hpp"
#define PERIODIC_WAITING_PERIOD pdMS_TO_TICKS(30)

#define PRIO_IDLE 0
#define PRIO_USER_COMMAND 1
#define PRIO_LOCO_TIMEOUT 1
#define PRIO_CREATE_BITSTREAM 2
#define PRIO_REFRESH 2
#define PRIO_TX 3

using namespace DCC;
static LocoRegistry::LocoRegistry registry;
QueueHandle_t stage2OutputQueue;
QueueHandle_t stage1InputQueue;
TaskHandle_t periodicSenderTaskHandle;
TaskHandle_t stage1ProcessorTaskHandle;

void vPeriodicSenderTask(void *pvParameter) {
    auto registry = static_cast<LocoRegistry::LocoRegistry *>(pvParameter);

    while (true) {
        registry->forEachDirtySlot([](const LocoRegistry::LocoSlot& loco) {
            if (loco.dirtyMask & LocoRegistry::ValidityMask::SPEED_VALID) {
                Packet pkt;
                Packets::SpeedPacketAssembler::build(loco.id, loco.speed, &pkt);
                xQueueSend(stage1InputQueue, &pkt, portMAX_DELAY);
            }
        });
        vTaskDelay(PERIODIC_WAITING_PERIOD);
    }
}

void vStage2ToPIOFifo(void *pvParameter) {
    Framing::BitstreamPacket *rxBuffer;

    while (true) {
        if (xQueueReceive(stage2OutputQueue, &rxBuffer, portMAX_DELAY)) {

        }
    }
}

void vStage1ProcessorTask(void *pvParameter) {
    Packet *rxBuffer;
    Framing::BitstreamPacket txBuffer;

    while (true) {
        if (xQueueReceive(stage1InputQueue, &rxBuffer, portMAX_DELAY)) {
            txBuffer.reinit(rxBuffer);
            xQueueSendToFront(stage2OutputQueue, &txBuffer, portMAX_DELAY);
        }
    }
}

int main() {
    stage2OutputQueue= xQueueCreate(16, sizeof(DCC::Framing::BitstreamPacket));
    stage1InputQueue = xQueueCreate(8, sizeof(DCC::Packet));

    xTaskCreate(
        vStage1ProcessorTask,
        "Packet to Bitstream Processor Task",
        configMINIMAL_STACK_SIZE,
        nullptr,
        PRIO_CREATE_BITSTREAM,
        &stage1ProcessorTaskHandle
    );

    xTaskCreate(
        vPeriodicSenderTask,
        "Periodic Sender",
        configMINIMAL_STACK_SIZE,
        &registry,
        PRIO_REFRESH,
        &periodicSenderTaskHandle
    );

    vTaskStartScheduler();
}
