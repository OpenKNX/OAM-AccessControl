#pragma once
#include <cstdlib>
#include <cstddef>
#include <algorithm>
inline size_t allocationCount = 0, largestAllocation = 0;
inline size_t failAt = static_cast<size_t>(-1), blockLimit = static_cast<size_t>(-1);
inline void* checkedAllocation(size_t size)
{
    largestAllocation = std::max(largestAllocation, size);
    if (allocationCount++ >= failAt || size > blockLimit) return nullptr;
    return std::malloc(size);
}
#define PSRAM_MALLOC(size) checkedAllocation(size)
