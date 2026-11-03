#pragma once
#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))
#ifndef ESP_IDF_VERSION
#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(5, 5, 4)
#endif
