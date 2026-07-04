#include "GameState.h"

#include <algorithm>
#include <cstdlib>

void GameState::Reset(GameMode newMode, int64_t newSeed, bool newHardMode, const Verse& verse) {
    mode = newMode;
    seed = newSeed;
    hardMode = newHardMode;
    currentStage = 0;
    gameOver = false;
    targetVerse = verse;
    history.clear();
}

std::string GameState::CheckHardModeViolation(const std::string& bookGuess,
                                               const std::vector<std::string>& digitGuesses,
                                               const BibleData& data) const {
    std::string requiredBook;
    std::string requiredArea;
    std::vector<std::string> requiredGreens(4);
    std::vector<bool> hasRequiredGreen(4, false);
    std::vector<std::string> requiredYellows;

    for (const auto& record : history) {
        if (record.bookColor == GuessColor::Green) {
            requiredBook = record.bookGuess;
        } else if (record.bookColor == GuessColor::Yellow && requiredArea.empty()) {
            requiredArea = data.getBookArea(record.bookGuess);
        }

        for (int i = 0; i < 4 && i < (int)record.digitColors.size(); ++i) {
            if (record.digitColors[i] == GuessColor::Green) {
                requiredGreens[i] = record.digitGuesses[i];
                hasRequiredGreen[i] = true;
            } else if (record.digitColors[i] == GuessColor::Yellow) {
                requiredYellows.push_back(record.digitGuesses[i]);
            }
        }
    }

    if (!requiredBook.empty() && bookGuess != requiredBook) {
        return "Hard mode: your guess must use the previously confirmed book.";
    }
    if (!requiredArea.empty() && data.getBookArea(bookGuess) != requiredArea) {
        return "Hard mode: your guess must match the previously hinted book area.";
    }
    for (int i = 0; i < 4; ++i) {
        if (hasRequiredGreen[i] && digitGuesses[i] != requiredGreens[i]) {
            return "Hard mode: your guess must keep previously revealed digits in the same positions.";
        }
    }
    for (const auto& yellowDigit : requiredYellows) {
        if (std::find(digitGuesses.begin(), digitGuesses.end(), yellowDigit) == digitGuesses.end()) {
            return "Hard mode: your guess must include previously revealed digits (yellow hints).";
        }
    }

    int chapter = std::atoi((digitGuesses[0] + digitGuesses[1]).c_str());
    int verseNum = std::atoi((digitGuesses[2] + digitGuesses[3]).c_str());
    if (!data.verseExists(bookGuess, chapter, verseNum)) {
        return "Hard mode: guessed verse must exist in the dataset.";
    }

    return "";
}
