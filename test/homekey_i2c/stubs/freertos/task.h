#pragma once
#include "FreeRTOS.h"
inline TickType_t ticks = 0;
inline TickType_t xTaskGetTickCount() { return ticks; }
inline void vTaskDelay(TickType_t delay) { ticks += delay; }
