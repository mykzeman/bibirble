#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "BibleData.h"
#include "GuessColor.h"

enum class GameMode { Daily, Random };

struct GuessRecord {
    std::string bookGuess;
    GuessColor bookColor = GuessColor::Gray;
    std::vector<std::string> digitGuesses;  // c1, c2, v1, v2
    std::vector<GuessColor> digitColors;    // c1, c2, v1, v2
};

// Single source of truth for an in-progress or finished game. Replaces the
// scattered m_targetVerse/m_currentStage/m_gameOver members that used to live
// directly on BibirbleWindow.
class GameState {
public:
    void Reset(GameMode newMode, int64_t newSeed, bool newHardMode, const Verse& verse);

    GameMode mode = GameMode::Daily;
    int64_t seed = 0;
    bool hardMode = false;
    int currentStage = 0;
    bool gameOver = false;
    Verse targetVerse;
    std::vector<GuessRecord> history;

    // Ports submit.js's hard-mode enforcement: checks a pending guess against
    // clues already given by earlier rows in history, plus dataset existence.
    // Returns a user-facing violation message, or an empty string if the
    // guess is allowed.
    std::string CheckHardModeViolation(const std::string& bookGuess,
                                        const std::vector<std::string>& digitGuesses,
                                        const BibleData& data) const;

    // Wordle-style emoji grid (🟩/🟨/⬛) built straight from history, plus the
    // citation line. Only rows actually played get a line -- unlike the web
    // version, which pads blank lines for unplayed rows on an early win.
    // Immune to the web's gray/grey emoji-key mismatch since there is only
    // ever one GuessColor.
    std::string BuildShareText() const;
};
