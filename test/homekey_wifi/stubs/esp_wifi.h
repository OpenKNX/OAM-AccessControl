#pragma once
inline constexpr int ESP_OK = 0, WIFI_STORAGE_FLASH = 1;
inline int storageResult = ESP_OK, selectedStorage = 0;
inline int esp_wifi_set_storage(int storage) { selectedStorage = storage; return storageResult; }
