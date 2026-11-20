#include "WordPuzzle.h"

#include <cctype>
#include <sstream>

namespace {
std::string Trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string Upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string Underscores(size_t n, const std::string& gap) {
    std::string out;
    for (size_t i = 0; i < n; ++i) out += i ? gap + "_" : "_";
    return out;
}
}  // namespace

std::string NormalizeWord(const std::string& word) {
    std::string out;
    for (char c : Trim(word)) {
        if (!std::isalpha(static_cast<unsigned char>(c))) return "";
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool StoryText::Parse(const std::string& raw, StoryText& out, std::string& error) {
    out = StoryText{};
    std::string current;
    size_t i = 0;
    while (i < raw.size()) {
        if (raw.compare(i, 2, "{{") == 0) {
            const size_t end = raw.find("}}", i + 2);
            if (end == std::string::npos) {
                error = "a \"{{\" blank is never closed with \"}}\"";
                return false;
            }
            const std::string inside = raw.substr(i + 2, end - i - 2);
            const size_t bar = inside.find('|');
            const std::string answerRaw = Trim(inside.substr(0, bar));
            StoryBlank blank;
            blank.answer = NormalizeWord(answerRaw);
            blank.hint = bar == std::string::npos ? "" : Trim(inside.substr(bar + 1));
            if (answerRaw.empty()) {
                error = "a blank has no answer";
                return false;
            }
            if (blank.answer.empty()) {
                error = "blank answer \"" + answerRaw + "\" must be one word of letters only";
                return false;
            }
            out.parts.push_back(current);
            out.blanks.push_back(blank);
            current.clear();
            i = end + 2;
        } else {
            current += raw[i++];
        }
    }
    out.parts.push_back(current);
    return true;
}

std::string StoryText::Render(size_t solved, const std::string& gap) const {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        out += parts[i];
        if (i >= blanks.size()) break;
        if (i < solved) {
            out += Upper(blanks[i].answer);
        } else if (i == solved) {
            out += "[" + gap + Underscores(blanks[i].answer.size(), gap) + gap + "]";
        } else {
            out += Underscores(blanks[i].answer.size(), gap);
        }
    }
    return out;
}

std::vector<GuessColor> ScoreWord(const std::string& guess, const std::string& answer) {
    std::vector<GuessColor> colors(guess.size(), GuessColor::Gray);
    if (guess.size() != answer.size()) return colors;
    int unmatched[26] = {0};
    for (size_t i = 0; i < guess.size(); ++i) {
        if (guess[i] == answer[i]) {
            colors[i] = GuessColor::Green;
        } else if (answer[i] >= 'a' && answer[i] <= 'z') {
            unmatched[answer[i] - 'a']++;
        }
    }
    for (size_t i = 0; i < guess.size(); ++i) {
        if (colors[i] == GuessColor::Green || guess[i] < 'a' || guess[i] > 'z') continue;
        int& left = unmatched[guess[i] - 'a'];
        if (left > 0) {
            colors[i] = GuessColor::Yellow;
            --left;
        }
    }
    return colors;
}

void WordList::Load(const std::string& text) {
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::string w = NormalizeWord(line);
        if (!w.empty() && w == Trim(line)) m_words.insert(w);
    }
}

void WordList::Add(const std::string& word) {
    const std::string w = NormalizeWord(word);
    if (!w.empty()) m_words.insert(w);
}

std::string DefaultHint(const std::string& answer) {
    if (answer.empty()) return "";
    return "It starts with " + std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(answer[0])))) +
           " and has " + std::to_string(answer.size()) + " letters.";
}

std::string WordHardModeViolation(const std::vector<WordGuess>& history, const std::string& guess) {
    auto upper = [](char c) { return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c)))); };
    for (const WordGuess& g : history) {
        for (size_t i = 0; i < g.word.size() && i < g.colors.size(); ++i) {
            if (g.colors[i] == GuessColor::Green && (i >= guess.size() || guess[i] != g.word[i])) {
                return "Hard mode: keep the green " + upper(g.word[i]) + " in spot " + std::to_string(i + 1) + ".";
            }
        }
        int needed[26] = {0};
        for (size_t i = 0; i < g.word.size() && i < g.colors.size(); ++i) {
            const char c = g.word[i];
            if (c >= 'a' && c <= 'z' && g.colors[i] != GuessColor::Gray) needed[c - 'a']++;
        }
        int have[26] = {0};
        for (char c : guess) {
            if (c >= 'a' && c <= 'z') have[c - 'a']++;
        }
        for (int c = 0; c < 26; ++c) {
            if (have[c] < needed[c]) {
                return "Hard mode: your guess must use the " + upper(static_cast<char>('a' + c)) +
                       (needed[c] > 1 ? " " + std::to_string(needed[c]) + " times." : " you found.");
            }
        }
    }
    return "";
}
