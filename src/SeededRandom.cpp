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

uint32_t MixSeed(int64_t seed) {
    uint32_t h = static_cast<uint32_t>(seed);
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h ? h : 0x9e3779b9u;  // xorshift gets stuck on 0
}

int64_t SeedFromText(const std::string& text) {
    // Number seeds (like the one shown after a game) are used as-is so a
    // shared seed replays the same verse, matching the web version.
    if (!text.empty() && text.size() <= 9 &&
        text.find_first_not_of("0123456789") == std::string::npos) {
        return std::stoll(text);
    }
    return HashStringToInt(text);
}

int PickIndexFromSeed(int64_t seed, int length) {
    SeededRandom random(MixSeed(seed));
    return static_cast<int>(std::floor(random.Next() * length));
}

std::string GetTodayLocalDateString() {
    std::time_t t = std::time(nullptr);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  localTm.tm_year + 1900, localTm.tm_mon + 1, localTm.tm_mday);
    return std::string(buf);
}

int64_t GetDailySeed() {
    return HashStringToInt(GetTodayLocalDateString());
}

int64_t GenerateRandomSeed() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dis(0, 999999999);
    return dis(gen);
}
