#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>
#include <map>
#include <set>
#include <string>

#include "WordPuzzle.h"

// Story Mode's full-screen story page (swapped in by BibirbleWindow like the
// map and game screens). Shows a piece of story text; if it has
// fill-in-the-blank puzzles, the player solves them one at a time,
// Wordle-style (unlimited guesses, real English words or a map answer, hint
// after 6 misses), and Continue only appears once all are solved.
// "Back to map" leaves without finishing. An optional header (e.g. a
// level's results) can sit above the story. An on-screen QWERTY keyboard
// (with Enter and Del) types guesses, and its keys turn green/yellow/gray
// like Wordle's as letters are tried.
class StoryScreen : public wxScrolledWindow {
public:
    using Callback = std::function<void()>;

    explicit StoryScreen(wxWindow* parent);

    // Rebuilds the page. `story` and `words` must outlive the page (they live
    // in BibirbleWindow's StoryMap / WordList). Either callback may be empty:
    // no "Back to map" button is shown without onLeave.
    void ShowStory(const wxString& title, const wxString& header, const StoryText& story, const WordList& words,
                   Callback onContinue, Callback onLeave);

private:
    void OnGuess();
    void AddGuessRow(const std::string& guess, const std::vector<GuessColor>& colors);
    void RefreshStory();
    void BuildKeyboard(wxSizer* layout);
    void OnKey(const wxString& key);
    void UpdateKeyColours(const std::string& guess, const std::vector<GuessColor>& colors);
    void ResetKeyColours();

    const StoryText* m_story = nullptr;
    const WordList* m_words = nullptr;
    Callback m_onContinue;
    Callback m_onLeave;
    size_t m_solved = 0;
    int m_wrongOnThisBlank = 0;
    std::set<std::string> m_guessedThisBlank;

    wxStaticText* m_storyText = nullptr;
    wxStaticText* m_howTo = nullptr;
    wxBoxSizer* m_guessSizer = nullptr;
    wxPanel* m_guessPanel = nullptr;
    wxPanel* m_inputRow = nullptr;
    wxTextCtrl* m_input = nullptr;
    wxStaticText* m_message = nullptr;
    wxStaticText* m_hint = nullptr;
    wxButton* m_continue = nullptr;
    wxPanel* m_keyboard = nullptr;
    std::map<char, wxButton*> m_keys;       // 'a'..'z'
    std::map<char, GuessColor> m_keyState;  // best clue so far for each letter on this blank
};
