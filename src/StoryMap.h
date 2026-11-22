#pragma once

#include <string>
#include <vector>

#include "BibleData.h"
#include "WordPuzzle.h"

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
    std::string region;  // StoryRegion id: where on the map the level's story happens
    // Story text, which may hold fill-in-the-blank puzzles ({{answer|hint}}):
    // before verse 1, after verse 1 and after verse 2 ("between"), and after
    // the level is passed.
    StoryText intro;
    std::vector<StoryText> between;
    StoryText outro;
    // The story shown before verse `verseIndex` (0 = intro, 1-2 = between).
    const StoryText* StoryBefore(int verseIndex) const;
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
    // False when the loaded Bible version doesn't have 3 of the level's verses
    // (an Old-Testament-only Bible has no Gospels): shown, but not playable.
    bool available = true;
    int MaxStars() const;
    bool Allows(BonusStar star) const;
};

struct StoryArea {
    std::string id;
    std::string title;
    std::string intro;
    bool openFromStart = false;
    bool bonusArea = false;     // hidden area, unlock: "perfect"
    // The book dropdown offers every book, not just this area's (the End
    // Times draws on the whole Bible).
    bool allBooks = false;
    std::vector<StoryLevel> levels;
    std::vector<StoryLevel> bonusLevels;
};

// A place on the Story map's picture (data/story_map.png) where levels sit:
// "egypt", "jerusalem"... x and y are percentages of the picture's width and
// height. A place nobody can pin down (the Garden of Eden, the Land of Uz) is
// "offMap": it's listed beside the map instead of guessed onto it. One whose
// spot is only traditional (Mount Sinai, the Red Sea crossing) is
// "approximate": pinned, but marked as approximate.
struct StoryRegion {
    std::string id;
    std::string name;
    double x = 0;
    double y = 0;
    bool offMap = false;
    bool approximate = false;
};

// The crowded land around Israel, shown zoomed in: a square of the picture
// (x, y = its top-left corner, size = its width and height, all percentages).
// On the whole map its regions share one pin at pinX, pinY; clicking it zooms
// in, where each region gets its own pin.
struct StoryCloseUp {
    std::string name;
    double x = 0, y = 0, size = 0;
    double pinX = 0, pinY = 0;
    bool Contains(const StoryRegion& r) const {
        return size > 0 && !r.offMap && r.x >= x && r.x <= x + size && r.y >= y && r.y <= y + size;
    }
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
    const std::vector<StoryRegion>& Regions() const { return m_regions; }
    const StoryCloseUp& CloseUp() const { return m_closeUp; }
    const StoryRegion* FindRegion(const std::string& id) const;
    const StoryLevel* FindLevel(const std::string& key) const;
    const StoryArea* FindArea(const std::string& id) const;
    const std::vector<std::string>& Errors() const { return m_errors; }
    // Every blank's answer in the map, so they can be guessed even when they
    // aren't dictionary words (names like NOAH).
    std::vector<std::string> AllAnswers() const;
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
    std::vector<StoryRegion> m_regions;
    StoryCloseUp m_closeUp;
    std::vector<std::string> m_errors;
    bool m_loaded = false;
};

// Turns one pool reference ("ruth", "genesis 6-12", "2chronicles 36",
// "ezekiel 24:15-27") into BibleData indices. Returns false if it can't be
// read or matches nothing.
bool ExpandPoolRef(const std::string& ref, const BibleData& bible,
                   std::vector<int>& out, std::string& error);
