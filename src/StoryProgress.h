#pragma once

#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "BibleData.h"
#include "GameState.h"
#include "StoryMap.h"

// The player's Story Mode progress: best stars per level, the verses solved
// toward each level's Perfect star, which levels are unlocked, and the level
// attempt in progress. Saved as the "story" key of PersistenceManager's file.
// No wx here, so it can be tested on its own (see tests/story_tests.cpp).

struct LevelStars {
    int guess = 0;          // 0-3, from total guesses
    bool hard = false;
    bool firstTry = false;
    bool noRetry = false;
    bool perfect = false;
    int Total() const;
    // Best of both, per category (best score is kept).
    LevelStars Merge(const LevelStars& other) const;
};

struct LevelProgress {
    bool passed = false;
    LevelStars stars;
    std::set<int> solved;   // BibleData indices solved in passed attempts
    // A failed (or abandoned) attempt since the last pass: the No retry star
    // can't be earned until the level is passed again.
    bool retryPending = false;
    int failedVerse = -1;   // kept in the next attempt's draw
};

// The level attempt being played (3 verses).
struct StoryRun {
    bool active = false;
    std::string levelKey;
    std::vector<int> verses;    // BibleData indices
    int solvedCount = 0;        // verses[0..solvedCount) are solved
    int totalGuesses = 0;       // across solved verses
    bool hardMode = false;
    bool anyFirstTry = false;
    std::vector<GuessRecord> currentGuesses;  // guesses so far on verses[solvedCount]
};

struct PassResult {
    LevelStars earned;      // this attempt
    LevelStars best;        // after merging with the saved best
    int perfectSolved = 0;  // pool verses solved so far
    int perfectTotal = 0;   // playable pool size
    bool hardModeUnlocked = false;  // just reached 4+ stars
    std::vector<std::string> newlyUnlocked;  // level keys
};

class StoryProgress {
public:
    static constexpr int kVersesPerLevel = 3;
    static constexpr int kHardModeStars = 4;

    void FromJson(const json& j, const BibleData& bible);
    json ToJson(const BibleData& bible) const;

    int TotalStars() const;
    const LevelProgress* Get(const std::string& key) const;
    LevelProgress& At(const std::string& key) { return m_levels[key]; }

    bool IsUnlocked(const std::string& key) const { return m_unlockedSet.count(key) > 0; }
    int UnlockedCount() const { return static_cast<int>(m_unlocked.size()); }
    // Unlocks every level whose rules are now met (repeating until nothing
    // changes). Levels never re-lock. Returns the keys unlocked by this call.
    std::vector<std::string> UpdateUnlocks(const StoryMap& map);
    // Stars a locked level needs right now (its formula at the current x).
    int StarsNeeded(const StoryLevel& level) const;
    // A bonus area (Ezekiel & Revelation) opens when every non-bonus level
    // has all its stars.
    bool AllRegularLevelsMaxed(const StoryMap& map) const;

    bool HardModeUnlocked(const std::string& key) const;

    // The verses a level can draw: its pool minus mature verses unless R18
    // mode is on (or the whole pool, if filtering leaves fewer than 3).
    static std::vector<int> PlayablePool(const StoryLevel& level, const BibleData& bible, bool allowMature);
    int PerfectProgress(const StoryLevel& level, const BibleData& bible, bool allowMature) const;

    // Starts (or replaces) the attempt for a level: draws 3 distinct random
    // verses, keeping the verse failed last time if there is one. Abandoning
    // a started attempt on another level counts as a retry for that level.
    void StartRun(const StoryLevel& level, const BibleData& bible, bool allowMature, bool hardMode,
                  std::mt19937& rng);
    StoryRun& Run() { return m_run; }
    const StoryRun& Run() const { return m_run; }
    void ClearRun() { m_run = StoryRun{}; }

    // The current verse was solved in `guesses` tries. Returns true when that
    // finished the level (call RecordPass next).
    bool RecordVerseSolved(int guesses);
    // The current verse ran out of guesses: the level fails and its next
    // attempt keeps that verse.
    void RecordFail();
    // All 3 verses solved: scores the level, keeps the best stars, unlocks.
    PassResult RecordPass(const StoryMap& map, const BibleData& bible, bool allowMature);

private:
    std::map<std::string, LevelProgress> m_levels;
    std::vector<std::string> m_unlocked;    // in unlock order
    std::set<std::string> m_unlockedSet;
    StoryRun m_run;

    void Unlock(const std::string& key, std::vector<std::string>& added);
    bool IsPassed(const std::string& key) const;
};

// "genesis 1:1": how a verse is stored in the save file, so saves survive
// changes to bible_sections.json's order.
std::string VerseKey(const Verse& v);
