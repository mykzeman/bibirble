#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

// Small JSON file under wxStandardPaths' per-user data directory, standing in
// for the web version's localStorage: remembers the last daily play date (for
// the once-a-day lockout), the last random seed (to prefill Settings), and
// Story Mode progress.
class PersistenceManager {
public:
    PersistenceManager();

    void Load();
    void Save() const;

    std::string GetLastPlayedDaily() const { return m_lastPlayedDaily; }
    void SetLastPlayedDaily(const std::string& localDateString);

    bool HasLastSeed() const { return m_hasLastSeed; }
    int64_t GetLastSeed() const { return m_lastSeed; }
    void SetLastSeed(int64_t seed);

    // Story Mode progress (StoryProgress::ToJson), saved under "story".
    const nlohmann::json& GetStory() const { return m_story; }
    void SetStory(const nlohmann::json& story);

private:
    std::string ResolveFilePath() const;

    std::string m_filePath;
    std::string m_lastPlayedDaily;
    bool m_hasLastSeed = false;
    int64_t m_lastSeed = 0;
    nlohmann::json m_story;
};
