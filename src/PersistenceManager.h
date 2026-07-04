#pragma once

#include <cstdint>
#include <string>

// Small JSON file under wxStandardPaths' per-user data directory, standing in
// for the web version's localStorage: remembers the last daily play date (for
// the once-a-day lockout) and the last random seed (to prefill Settings).
class PersistenceManager {
public:
    PersistenceManager();

    void Load();
    void Save() const;

    std::string GetLastPlayedDaily() const { return m_lastPlayedDaily; }
    void SetLastPlayedDaily(const std::string& utcDateString);

    bool HasLastSeed() const { return m_hasLastSeed; }
    int64_t GetLastSeed() const { return m_lastSeed; }
    void SetLastSeed(int64_t seed);

private:
    std::string ResolveFilePath() const;

    std::string m_filePath;
    std::string m_lastPlayedDaily;
    bool m_hasLastSeed = false;
    int64_t m_lastSeed = 0;
};
