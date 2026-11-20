#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <set>
#include <string>

#include "WordPuzzle.h"

// Shows a piece of Story Mode story text. If it has fill-in-the-blank
// puzzles, the player solves them one at a time, Wordle-style (unlimited
// guesses, real English words or a map answer, hint after 6 misses), and
// can only Continue once all are solved. "Back to map" leaves without
// finishing (ShowModal returns wxID_CANCEL; Continue returns wxID_OK).
class StoryPuzzleDialog : public wxDialog {
public:
    StoryPuzzleDialog(wxWindow* parent, const wxString& title, const StoryText& story, const WordList& words,
                      bool allowLeave = true);

private:
    void OnGuess();
    void AddGuessRow(const std::string& guess, const std::vector<GuessColor>& colors);
    void RefreshStory();

    const StoryText& m_story;
    const WordList& m_words;
    size_t m_solved = 0;
    int m_wrongOnThisBlank = 0;
    std::set<std::string> m_guessedThisBlank;

    wxStaticText* m_storyText = nullptr;
    wxStaticText* m_howTo = nullptr;
    wxScrolledWindow* m_guessArea = nullptr;
    wxBoxSizer* m_guessSizer = nullptr;
    wxPanel* m_inputRow = nullptr;
    wxTextCtrl* m_input = nullptr;
    wxStaticText* m_message = nullptr;
    wxStaticText* m_hint = nullptr;
    wxButton* m_continue = nullptr;
};
