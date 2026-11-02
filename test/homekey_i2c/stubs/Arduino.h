#pragma once
#include <array>
#include <cassert>
inline constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1;
inline std::array<int, 40> levels{}, modes{};
inline void digitalWrite(int pin, int level) { assert(modes.at(pin) == OUTPUT); levels.at(pin) = level; }
inline int digitalRead(int pin) { return levels.at(pin); }
inline void pinMode(int pin, int mode) { modes.at(pin) = mode; }
