/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdbool.h>

#define BTN_PIN_R 28
#define BTN_PIN_Y 21

#define LED_PIN_R 5
#define LED_PIN_Y 10

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void btn_callback(uint gpio, uint32_t events) {
    if ((events & GPIO_IRQ_EDGE_FALL) == 0) {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(xQueueBtn, &gpio, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void led_r_task(void* p) {
    (void)p;
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    bool is_blinking = false;
    bool is_on = false;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) {
            is_blinking = !is_blinking;
            if (!is_blinking) {
                is_on = false;
                gpio_put(LED_PIN_R, 0);
            }
        }

        if (is_blinking) {
            is_on = !is_on;
            gpio_put(LED_PIN_R, is_on);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void led_y_task(void* p) {
    (void)p;
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    bool is_blinking = false;
    bool is_on = false;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) {
            is_blinking = !is_blinking;
            if (!is_blinking) {
                is_on = false;
                gpio_put(LED_PIN_Y, 0);
            }
        }

        if (is_blinking) {
            is_on = !is_on;
            gpio_put(LED_PIN_Y, is_on);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_task(void* p) {
    (void)p;
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    uint gpio = 0;
    TickType_t last_r = 0;
    TickType_t last_y = 0;

    while (true) {
        if (xQueueReceive(xQueueBtn, &gpio, portMAX_DELAY) == pdTRUE) {
            TickType_t now = xTaskGetTickCount();

            if (gpio == BTN_PIN_R) {
                if ((now - last_r) > pdMS_TO_TICKS(30)) {
                    xSemaphoreGive(xSemaphoreLedR);
                    last_r = now;
                }
            } else if (gpio == BTN_PIN_Y) {
                if ((now - last_y) > pdMS_TO_TICKS(30)) {
                    xSemaphoreGive(xSemaphoreLedY);
                    last_y = now;
                }
            }
        }
    }
}



int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(8, sizeof(uint));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(led_r_task, "LED_R_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y_Task", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}