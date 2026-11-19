#pragma once

#include <string>
#include <vector>

#include "BibleData.h"

// Story Mode's map, loaded from story_map.json (see docs/STORY_MODE.md).
// Nothing about the areas is hard-coded: the JSON decides which areas and
// levels exist, their order, story text, verse pools, star rules and unlocks.

// A star requirement: a plain number, or "ax+b" where x = levels unlocked in
// the whole game. Values below 0 count as 0.
struct StarFormula {
    int a = 0;
    int b = 0;
    int Eval(int x) const;
    // Parses 64, "64", "2x", "3x+1", "6x-10". Returns false if unreadable.
    static bool Parse(const json& value, StarFormula& out);
};

// The most total guesses (across a level's 3 verses) allowed for 3, 2 and
// 1 guess stars. More guesses than tier1 is still a pass, with 0 guess stars.
struct GuessTiers {
    int tier3 = 6;
    int tier2 = 9;
    int tier1 = 12;
    int StarsFor(int totalGuesses) const;
};

enum class BonusStar { Hard, FirstTry, NoRetry, Perfect };

struct StoryLevel {
    std::string key;    // "areaId.levelId": what save data uses
    std::string id;
    std::string areaId;
    std::string title;
    std::string intro;
    std::string outro;
    std::vector<int> pool;  // BibleData indices, in dataset order, no duplicates
    StarFormula minStars;   // main-path levels
    StarFormula requiresStars;  // bonus levels
    GuessTiers guessTiers;
    GuessTiers hardGuessTiers;
    bool allowHard = true;
    bool allowFirstTry = true;
    bool allowNoRetry = true;
    bool allowPerfect = true;
    bool isBonus = false;       // one of an area's bonusLevels
    int MaxStars() const;
    bool Allows(BonusStar star) const;
};

struct StoryArea {
    std::string id;
    std::string title;
    std::string intro;
    bool openFromStart = false;
    bool bonusArea = false;     // hidden area, unlock: "perfect"
    std::vector<StoryLevel> levels;
    std::vector<StoryLevel> bonusLevels;
};

class StoryMap {
public:
    // Parses the map JSON and resolves every pool against the loaded Bible.
    // Problems (unknown book, too-small pool, bad formula, duplicate id...)
    // go into Errors(); the map is only usable if Errors() is empty.
    bool LoadFromText(const std::string& text, const BibleData& bible);

    // story_map.json on disk wins (so edits can be tested without
    // rebuilding), otherwise the copy compiled into the program.
    bool Load(const BibleData& bible);

    const std::vector<StoryArea>& Areas() const { return m_areas; }
    const StoryLevel* FindLevel(const std::string& key) const;
    const StoryArea* FindArea(const std::string& id) const;
    const std::vector<std::string>& Errors() const { return m_errors; }
    bool IsLoaded() const { return m_loaded; }

    // Paths through the map: each starts at the first area or at an
    // openFromStart area and runs through the areas after it (in JSON order)
    // until the next path start or bonus area. Level keys, in play order.
    std::vector<std::vector<std::string>> Paths() const;

private:
    bool ParseLevel(const json& j, const std::string& areaId, bool isBonus,
                    const GuessTiers& defaultTiers, double hardScale,
                    const BibleData& bible, StoryLevel& out);

    std::vector<StoryArea> m_areas;
    std::vector<std::string> m_errors;
    bool m_loaded = false;
};

// Turns one pool reference ("ruth", "genesis 6-12", "2chronicles 36",
// "ezekiel 24:15-27") into BibleData indices. Returns false if it can't be
// read or matches nothing.
bool ExpandPoolRef(const std::string& ref, const BibleData& bible,
                   std::vector<int>& out, std::string& error);
