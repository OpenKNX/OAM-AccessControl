#pragma once
using esp_err_t = int;
inline constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_NO_MEM = 0x101,
    ESP_ERR_INVALID_ARG = 0x102, ESP_ERR_INVALID_STATE = 0x103,
    ESP_ERR_INVALID_RESPONSE = 0x108, ESP_ERR_TIMEOUT = 0x107;
