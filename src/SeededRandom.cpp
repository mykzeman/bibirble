#include "SeededRandom.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>
#include <utility>
#include <vector>

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

void GetTodayUtcDate(int& year, int& month, int& day) {
    std::time_t t = std::time(nullptr);
    std::tm utcTm{};
#if defined(_WIN32)
    gmtime_s(&utcTm, &t);
#else
    gmtime_r(&t, &utcTm);
#endif
    year = utcTm.tm_year + 1900;
    month = utcTm.tm_mon + 1;
    day = utcTm.tm_mday;
}

std::string GetTodayUtcDateString() {
    int year, month, day;
    GetTodayUtcDate(year, month, day);
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
    return std::string(buf);
}

bool IsChristmasSeason() {
    int year, month, day;
    GetTodayUtcDate(year, month, day);
    return month == 12;
}

int PickChristmasDailyPosition(int poolLength, int specialPosition, int year, int day) {
    if (day == 25 && specialPosition != -1) return specialPosition;
    std::vector<int> positions;
    for (int k = 0; k < poolLength; ++k) {
        if (k != specialPosition) positions.push_back(k);
    }
    if (positions.empty()) return 0;
    SeededRandom random(MixSeed(HashStringToInt("christmas " + std::to_string(year))));
    for (int k = (int)positions.size() - 1; k > 0; --k) {
        int j = static_cast<int>(std::floor(random.Next() * (k + 1)));
        std::swap(positions[k], positions[j]);
    }
    return positions[(day - 1) % positions.size()];
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
