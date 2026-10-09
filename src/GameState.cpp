#include "GameState.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <sstream>

namespace {
const char* EmojiFor(GuessColor color) {
    switch (color) {
        case GuessColor::Green:  return "\xF0\x9F\x9F\xA9";  // 🟩
        case GuessColor::Yellow: return "\xF0\x9F\x9F\xA8";  // 🟨
        case GuessColor::Gray:
        default:                 return "\xE2\xAC\x9B";      // ⬛
    }
}
}  // namespace

void GameState::Reset(GameMode newMode, int64_t newSeed, bool newHardMode, bool newBookHints, const Verse& verse) {
    mode = newMode;
    seed = newSeed;
    hardMode = newHardMode;
    bookHints = newBookHints && !newHardMode;
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

std::string GameState::BuildShareText() const {
    std::ostringstream out;
    out << "Could you beat this score in Bibirble?\n\n";
    out << "- " << targetVerse.book << " " << targetVerse.chapter << ":" << targetVerse.verse << "\n\n";

    for (const auto& record : history) {
        out << EmojiFor(record.bookColor);
        for (GuessColor color : record.digitColors) {
            out << EmojiFor(color);
        }
        out << "\n";
    }

    return out.str();
}

std::vector<std::string> GameState::FilterBooksByClues(const std::vector<std::string>& allBooks,
                                                       const BibleData& data) const {
    std::set<std::string> wrongBooks;
    std::set<std::string> grayAreas;
    std::string yellowArea;
    for (const auto& record : history) {
        if (record.bookColor == GuessColor::Green) {
            return {record.bookGuess};
        }
        wrongBooks.insert(record.bookGuess);
        if (record.bookColor == GuessColor::Yellow && yellowArea.empty()) {
            yellowArea = data.getBookArea(record.bookGuess);
        } else if (record.bookColor == GuessColor::Gray) {
            grayAreas.insert(data.getBookArea(record.bookGuess));
        }
    }

    std::vector<std::string> books;
    for (const auto& book : allBooks) {
        if (wrongBooks.count(book)) continue;
        std::string area = data.getBookArea(book);
        if (!yellowArea.empty() ? area != yellowArea : grayAreas.count(area) > 0) continue;
        books.push_back(book);
    }
    return books;
}
