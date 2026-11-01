#pragma once
#include <cstdint>

class IPAddress
{
  public:
    explicit IPAddress(uint32_t address = 0) : value(address) {}
    uint32_t value;
};
