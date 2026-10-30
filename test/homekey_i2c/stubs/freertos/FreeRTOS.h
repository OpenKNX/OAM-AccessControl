#pragma once
#include <cstdint>
using TickType_t = uint32_t;
inline constexpr TickType_t portMAX_DELAY = UINT32_MAX;
#define pdMS_TO_TICKS(ms) (ms)
