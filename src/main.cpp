#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "locoregistry.hpp"
#include "framing.hpp"
#include "functiongroups.hpp"
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

void periodicFunction(const LocoRegistry::LocoSlot& loco) {
    Packet pkt;
    if (loco.dirtyMask & LocoRegistry::ValidityMask::SPEED_VALID != 0) {
        Packets::SpeedPacketAssembler::build(loco.id, loco.speed, &pkt);
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
    } else if (loco.dirtyMask & LocoRegistry::ValidityMask::FUNCTION_1_VALID != 0) {
        Packets::FunctionGroupBuilder::buildFG1(loco.f0_f4State, loco.id, &pkt);
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
    } else if (loco.dirtyMask & LocoRegistry::ValidityMask::FUNCTION_2L_VALID != 0) {
        Packets::FunctionGroupBuilder::buildFG2(loco.f5_f12State, loco.id, Packets::LOW, &pkt);
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
    } else if (loco.dirtyMask & LocoRegistry::ValidityMask::FUNCTION_3_VALID != 0) {
        Packets::FunctionGroupBuilder::buildFG3(loco.f13_f20State, loco.id, &pkt);
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
        xQueueSend(stage1InputQueue, &pkt, pdMS_TO_TICKS(10));
    }
}

void vPeriodicSenderTask(void *pvParameter) {
    auto registry = static_cast<LocoRegistry::LocoRegistry *>(pvParameter);

    while (true) {
        registry->forEachDirtySlot(periodicFunction);
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
