#pragma once

#include <cstdint>
#include <string>

// Bit-for-bit port of Bibirble-web's scripts/data.js seed algorithm, so the
// daily puzzle and shared seeds land on the exact same verse on both platforms.

// JS: hash = (hash << 5) - hash + charCode; hash |= 0; return Math.abs(hash)
// hash*31 + charCode computed mod 2^32 is equivalent to the JS per-char ToInt32
// wraparound; the final abs() is done in int64 so abs(INT32_MIN) can't overflow.
int64_t HashStringToInt(const std::string& str);

// JS: createSeededRandom(seed) -> xorshift32 generator
class SeededRandom {
public:
    explicit SeededRandom(int64_t seed);
    double Next();

private:
    uint32_t m_value;
};

// JS: pickIndexFromSeed(seed, length) -- exactly one RNG call
int PickIndexFromSeed(int64_t seed, int length);

// JS: new Date().toISOString().slice(0, 10), computed in UTC
std::string GetTodayUtcDateString();

// JS: getDailySeed() -- hashStringToInt(today)
int64_t GetDailySeed();

// Generates a fresh, non-deterministic seed for a blank "Random" seed field.
// JS: Math.floor(Math.random() * 1e9)
int64_t GenerateRandomSeed();
