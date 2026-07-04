#include "SeededRandom.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>

int64_t HashStringToInt(const std::string& str) {
    uint32_t hash = 0;
    for (unsigned char c : str) {
        hash = hash * 31u + static_cast<uint32_t>(c);
    }
    int32_t signedHash = static_cast<int32_t>(hash);
    return std::abs(static_cast<int64_t>(signedHash));
}

SeededRandom::SeededRandom(int64_t seed)
    : m_value(static_cast<uint32_t>(seed)) {}

double SeededRandom::Next() {
    m_value ^= m_value << 13;
    m_value ^= m_value >> 17;
    m_value ^= m_value << 5;
    return static_cast<double>(m_value) / 4294967296.0;  // 0x100000000
}

int PickIndexFromSeed(int64_t seed, int length) {
    SeededRandom random(seed);
    return static_cast<int>(std::floor(random.Next() * length));
}

std::string GetTodayUtcDateString() {
    std::time_t t = std::time(nullptr);
    std::tm utcTm{};
#if defined(_WIN32)
    gmtime_s(&utcTm, &t);
#else
    gmtime_r(&t, &utcTm);
#endif
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  utcTm.tm_year + 1900, utcTm.tm_mon + 1, utcTm.tm_mday);
    return std::string(buf);
}

int64_t GetDailySeed() {
    return HashStringToInt(GetTodayUtcDateString());
}

int64_t GenerateRandomSeed() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dis(0, 999999999);
    return dis(gen);
}
