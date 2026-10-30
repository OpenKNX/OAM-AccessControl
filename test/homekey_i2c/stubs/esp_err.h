#pragma once
using esp_err_t = int;
inline constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_INVALID_ARG = 1,
    ESP_ERR_INVALID_STATE = 2, ESP_ERR_NO_MEM = 3, ESP_ERR_TIMEOUT = 4;
