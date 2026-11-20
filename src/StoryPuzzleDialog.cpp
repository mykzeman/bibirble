#include "StoryPuzzleDialog.h"
#include "Theme.h"

#include <cctype>

namespace {
const int kTextWidth = 420;       // instructions, hint
const int kStoryPadding = 12;     // around the story text inside its card
const int kStoryWidth = kTextWidth - 2 * kStoryPadding;

wxColour TileColour(GuessColor c) {
    switch (c) {
        case GuessColor::Green: return wxColour(0, 200, 0);
        case GuessColor::Yellow: return wxColour(255, 230, 0);
        case GuessColor::Gray:
        default: return wxColour(190, 190, 190);
    }
}
}  // namespace

StoryPuzzleDialog::StoryPuzzleDialog(wxWindow* parent, const wxString& title, const StoryText& story,
                                     const WordList& words, bool allowLeave)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      m_story(story), m_words(words) {
    SetBackgroundColour(Theme::Background());
    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* heading = new wxStaticText(this, wxID_ANY, title);
    heading->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    layout->Add(heading, 0, wxALL, 10);

    wxPanel* card = new wxPanel(this);
    card->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* cardLayout = new wxBoxSizer(wxVERTICAL);
    m_storyText = new wxStaticText(card, wxID_ANY, "");
    m_storyText->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    cardLayout->Add(m_storyText, 0, wxALL, kStoryPadding);
    // Fixed width, so the text always wraps to fit inside the card.
    cardLayout->SetMinSize(wxSize(kTextWidth, -1));
    card->SetSizer(cardLayout);
    layout->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    const bool hasPuzzle = !story.blanks.empty();
    if (hasPuzzle) {
        m_howTo = new wxStaticText(this, wxID_ANY,
            "Fill in the blank! Guess the missing word: green = right letter, right spot; "
            "yellow = in the word somewhere else; gray = not in the word.");
        m_howTo->Wrap(kTextWidth);
        layout->Add(m_howTo, 0, wxALL, 10);

        m_guessArea = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 150));
        m_guessArea->SetScrollRate(0, 10);
        m_guessArea->SetBackgroundColour(Theme::Background());
        m_guessSizer = new wxBoxSizer(wxVERTICAL);
        m_guessArea->SetSizer(m_guessSizer);
        layout->Add(m_guessArea, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        m_inputRow = new wxPanel(this);
        wxBoxSizer* inputLayout = new wxBoxSizer(wxHORIZONTAL);
        m_input = new wxTextCtrl(m_inputRow, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        m_input->SetFont(wxFont(14, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnGuess(); });
        inputLayout->Add(m_input, 1, wxEXPAND | wxRIGHT, 6);
        wxButton* guessBtn = new wxButton(m_inputRow, wxID_ANY, "Guess");
        guessBtn->SetBackgroundColour(Theme::Accent());
        guessBtn->SetForegroundColour(Theme::AccentText());
        guessBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnGuess(); });
        inputLayout->Add(guessBtn, 0);
        m_inputRow->SetSizer(inputLayout);
        layout->Add(m_inputRow, 0, wxEXPAND | wxALL, 10);

        m_message = new wxStaticText(this, wxID_ANY, "");
        m_message->SetForegroundColour(wxColour(170, 30, 30));
        layout->Add(m_message, 0, wxLEFT | wxRIGHT, 10);
        m_hint = new wxStaticText(this, wxID_ANY, "");
        layout->Add(m_hint, 0, wxALL, 10);
    }

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    if (allowLeave) {
        wxButton* leave = new wxButton(this, wxID_CANCEL, "Back to map");
        buttons->Add(leave, 0, wxALL, 5);
    }
    buttons->AddStretchSpacer();
    m_continue = new wxButton(this, wxID_OK, "Continue");
    m_continue->SetBackgroundColour(Theme::Accent());
    m_continue->SetForegroundColour(Theme::AccentText());
    m_continue->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    buttons->Add(m_continue, 0, wxALL, 5);
    layout->Add(buttons, 0, wxEXPAND | wxALL, 5);

    // Without leaving, the window's close box mustn't skip a required puzzle.
    if (!allowLeave) {
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
            if (m_solved < m_story.blanks.size() && e.CanVeto()) e.Veto();
            else e.Skip();
        });
    }

    SetSizer(layout);
    RefreshStory();
    if (m_input) m_input->SetFocus();
}

void StoryPuzzleDialog::RefreshStory() {
    m_storyText->SetLabel(wxString::FromUTF8(m_story.Render(m_solved, "\xC2\xA0")));  // no-break space
    m_storyText->Wrap(kStoryWidth);
    // The wrapped label is taller now: make its card (and the dialog) grow.
    m_storyText->InvalidateBestSize();
    m_storyText->GetParent()->InvalidateBestSize();
    m_storyText->GetParent()->Layout();
    const bool done = m_solved >= m_story.blanks.size();
    // Continue only appears once every blank is solved.
    m_continue->Show(done);
    if (m_inputRow) {
        m_howTo->Show(!done);
        m_guessArea->Show(!done);
        m_inputRow->Show(!done);
        if (!done) {
            const size_t len = m_story.blanks[m_solved].answer.size();
            m_input->SetMaxLength(static_cast<unsigned long>(len));
            m_input->SetHint(wxString::Format("%zu letters", len));
        }
    }
    if (m_hint) m_hint->SetLabel("");
    Layout();
    Fit();
    if (done) m_continue->SetFocus();
}

void StoryPuzzleDialog::AddGuessRow(const std::string& guess, const std::vector<GuessColor>& colors) {
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    for (size_t i = 0; i < guess.size(); ++i) {
        wxStaticText* tile = new wxStaticText(m_guessArea, wxID_ANY, wxString(1, (wxChar)std::toupper(guess[i])),
                                              wxDefaultPosition, wxSize(30, 30), wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);
        tile->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        tile->SetBackgroundColour(TileColour(colors[i]));
        row->Add(tile, 0, wxALL, 2);
    }
    m_guessSizer->Add(row, 0, wxLEFT, 2);
    m_guessArea->FitInside();
    m_guessArea->Scroll(0, m_guessArea->GetVirtualSize().y);
    m_guessArea->Layout();
}

void StoryPuzzleDialog::OnGuess() {
    if (m_solved >= m_story.blanks.size()) return;
    const StoryBlank& blank = m_story.blanks[m_solved];
    const std::string guess = NormalizeWord(std::string(m_input->GetValue().utf8_str()));

    // A rejected guess stays selected, so typing the next one replaces it.
    auto reject = [this](const wxString& why) {
        m_message->SetLabel(why);
        m_input->SelectAll();
        m_input->SetFocus();
    };
    if (guess.size() != blank.answer.size()) {
        reject(wxString::Format("The word has %zu letters.", blank.answer.size()));
        return;
    }
    if (guess != blank.answer && !m_words.Contains(guess)) {
        reject("That's not in the word list. Try a real English word.");
        return;
    }
    if (m_guessedThisBlank.count(guess)) {
        reject("You already tried that one.");
        return;
    }
    m_message->SetLabel("");
    m_guessedThisBlank.insert(guess);
    m_input->Clear();
    AddGuessRow(guess, ScoreWord(guess, blank.answer));

    if (guess == blank.answer) {
        ++m_solved;
        m_wrongOnThisBlank = 0;
        m_guessedThisBlank.clear();
        m_guessSizer->Clear(true);
        RefreshStory();
        return;
    }
    if (++m_wrongOnThisBlank >= kGuessesBeforeHint) {
        const std::string hint = blank.hint.empty() ? DefaultHint(blank.answer) : blank.hint;
        m_hint->SetLabel(wxString::FromUTF8("\xF0\x9F\x92\xA1 Hint: ") + wxString::FromUTF8(hint));  // 💡
        m_hint->Wrap(kTextWidth);
    }
    Layout();
    Fit();
}
