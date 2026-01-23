#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "registry.hpp"
#define PERIODIC_WAITING_PERIOD pdMS_TO_TICKS(30)

static DCC::Registry::LocoRegistry registry;
QueueHandle_t packetOutputQueue;
TaskHandle_t periodicSenderTaskHandle;

void vPeriodicSenderTask(void *pvParameter) {
    auto registry = static_cast<DCC::Registry::LocoRegistry *>(pvParameter);

    while (true) {
        xSemaphoreTake(registry->semaphore, portMAX_DELAY);
        registry->forEachActiveSlot([](DCC::Registry::LocoSlot& loco) {
            xQueueSendToBack(packetOutputQueue, &loco.speedState, portMAX_DELAY);
        });
        xSemaphoreGive(registry->semaphore);
        vTaskDelay(PERIODIC_WAITING_PERIOD);
    }
}

void vPacketQueueSendToPIoFifo();

int main() {
    xQueueCreate(16, sizeof(DCC::Packet));
    xTaskCreate(
        vPeriodicSenderTask,
        "Periodic Sender",
        configMINIMAL_STACK_SIZE,
        &registry,
        configMAX_PRIORITIES - 2,
        &periodicSenderTaskHandle
    );
    vTaskStartScheduler();
}
