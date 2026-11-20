#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "GuessColor.h"

// Fill-in-the-blank word puzzles inside Story Mode's story text
// (docs/STORY_MODE.md §3.5). In story_map.json a blank is written
//   {{answer}}  or  {{answer|hint shown after 6 wrong guesses}}
// e.g. "God told Noah to build an {{ark|A huge wooden boat}}."
// Each blank is a Wordle-style puzzle: unlimited guesses of the answer's
// length, green/yellow/gray letters, and it must be solved to continue.
// No wx here, so it can be tested on its own.

struct StoryBlank {
    std::string answer;     // lowercase letters
    std::string hint;       // may be empty (a default hint is used then)
};

// A piece of story text split around its blanks:
// parts[0] blank[0] parts[1] blank[1] ... parts[n].
struct StoryText {
    std::vector<std::string> parts;
    std::vector<StoryBlank> blanks;

    bool Empty() const { return blanks.empty() && (parts.empty() || parts.front().empty()); }
    // The text with the first `solved` blanks filled in (in capitals) and the
    // rest shown as "_ _ _" (the next one to solve in [brackets]). `gap` goes
    // between the underscores (a non-breaking space keeps a blank on one line).
    std::string Render(size_t solved, const std::string& gap = " ") const;
    // Parses raw story text. Returns false (with an error) for an unclosed
    // "{{", an empty answer, or an answer that isn't letters only.
    static bool Parse(const std::string& raw, StoryText& out, std::string& error);
};

// Wordle colouring for a guess of the same length as the answer: green =
// right letter, right place; yellow = in the word elsewhere (counting
// repeats); gray = not (or no more) in the word. Both lowercase.
std::vector<GuessColor> ScoreWord(const std::string& guess, const std::string& answer);

// Lowercases and trims; returns "" if anything but letters is left.
std::string NormalizeWord(const std::string& word);

// The English words a guess may be (data/words.txt: the public-domain ENABLE
// list, 172,823 words, no names), plus extras
// such as every puzzle answer in the map, so Bible names that aren't in the
// dictionary can still be guessed.
class WordList {
public:
    // One word per line; anything that isn't lowercase letters is skipped.
    void Load(const std::string& text);
    void Add(const std::string& word);
    bool Contains(const std::string& word) const { return m_words.count(word) > 0; }
    size_t Size() const { return m_words.size(); }

private:
    std::unordered_set<std::string> m_words;
};

// The default hint when the author didn't write one.
std::string DefaultHint(const std::string& answer);

// Wrong guesses before the hint appears.
constexpr int kGuessesBeforeHint = 6;
