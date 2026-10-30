#pragma once
#include <array>
inline constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1;
inline std::array<int, 40> levels{}, modes{};
inline void digitalWrite(int pin, int level) { levels.at(pin) = level; }
inline int digitalRead(int pin) { return levels.at(pin); }
inline void pinMode(int pin, int mode) { modes.at(pin) = mode; }
