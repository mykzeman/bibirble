#include "StoryScreen.h"
#include "Theme.h"

#include <cctype>

namespace {
const int kTextWidth = 440;       // instructions, hint, header
const int kStoryPadding = 14;     // around the story text inside its card
const int kStoryWidth = kTextWidth - 2 * kStoryPadding;

wxFont Font(int size, bool bold = false) {
    return wxFont(size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
}

wxButton* AccentButton(wxWindow* parent, const wxString& label, int fontSize) {
    wxButton* btn = new wxButton(parent, wxID_ANY, label);
    btn->SetBackgroundColour(Theme::Accent());
    btn->SetForegroundColour(Theme::AccentText());
    btn->SetFont(Font(fontSize, true));
    return btn;
}

wxColour TileColour(GuessColor c) {
    switch (c) {
        case GuessColor::Green: return wxColour(0, 200, 0);
        case GuessColor::Yellow: return wxColour(255, 230, 0);
        case GuessColor::Gray:
        default: return wxColour(190, 190, 190);
    }
}
}  // namespace

StoryScreen::StoryScreen(wxWindow* parent) : wxScrolledWindow(parent) {
    SetScrollRate(0, 10);
    SetBackgroundColour(Theme::Background());
}

void StoryScreen::ShowStory(const wxString& title, const wxString& header, const StoryText& story,
                            const WordList& words, Callback onContinue, Callback onLeave) {
    m_story = &story;
    m_words = &words;
    m_onContinue = std::move(onContinue);
    m_onLeave = std::move(onLeave);
    m_solved = 0;
    m_wrongOnThisBlank = 0;
    m_guessedThisBlank.clear();
    m_howTo = nullptr;
    m_guessPanel = nullptr;
    m_guessSizer = nullptr;
    m_inputRow = nullptr;
    m_input = nullptr;
    m_message = nullptr;
    m_hint = nullptr;

    Freeze();
    DestroyChildren();
    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* heading = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("\xF0\x9F\x93\x96 ") + title);  // 📖
    heading->SetFont(Font(18, true));
    layout->Add(heading, 0, wxALL, 10);

    if (!header.IsEmpty()) {
        wxPanel* card = new wxPanel(this);
        card->SetBackgroundColour(*wxWHITE);
        wxBoxSizer* cs = new wxBoxSizer(wxVERTICAL);
        wxStaticText* h = new wxStaticText(card, wxID_ANY, header);
        h->SetFont(Font(12));
        h->Wrap(kStoryWidth);
        cs->Add(h, 0, wxALL, kStoryPadding);
        card->SetSizer(cs);
        layout->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    }

    // The story itself (hidden when there's no story text, e.g. a results page).
    if (!story.Empty()) {
        wxPanel* card = new wxPanel(this);
        card->SetBackgroundColour(*wxWHITE);
        wxBoxSizer* cardLayout = new wxBoxSizer(wxVERTICAL);
        m_storyText = new wxStaticText(card, wxID_ANY, "");
        m_storyText->SetFont(Font(14));
        cardLayout->Add(m_storyText, 0, wxALL, kStoryPadding);
        cardLayout->SetMinSize(wxSize(kTextWidth, -1));
        card->SetSizer(cardLayout);
        layout->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    } else {
        m_storyText = nullptr;
    }

    if (!story.blanks.empty()) {
        m_howTo = new wxStaticText(this, wxID_ANY,
            "Fill in the blank! Guess the missing word: green = right letter, right spot; "
            "yellow = in the word somewhere else; gray = not in the word.");
        m_howTo->Wrap(kTextWidth);
        layout->Add(m_howTo, 0, wxALL, 10);

        m_inputRow = new wxPanel(this);
        wxBoxSizer* inputLayout = new wxBoxSizer(wxHORIZONTAL);
        m_input = new wxTextCtrl(m_inputRow, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        m_input->SetFont(wxFont(16, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnGuess(); });
        inputLayout->Add(m_input, 1, wxEXPAND | wxRIGHT, 6);
        wxButton* guessBtn = AccentButton(m_inputRow, "Guess", 13);
        guessBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnGuess(); });
        inputLayout->Add(guessBtn, 0, wxEXPAND);
        m_inputRow->SetSizer(inputLayout);
        layout->Add(m_inputRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        m_message = new wxStaticText(this, wxID_ANY, "");
        m_message->SetForegroundColour(wxColour(170, 30, 30));
        layout->Add(m_message, 0, wxLEFT | wxRIGHT | wxTOP, 10);
        m_hint = new wxStaticText(this, wxID_ANY, "");
        m_hint->SetFont(Font(11, true));
        layout->Add(m_hint, 0, wxLEFT | wxRIGHT | wxTOP, 10);

        // Guesses so far, newest at the bottom (the whole page scrolls).
        m_guessPanel = new wxPanel(this);
        m_guessSizer = new wxBoxSizer(wxVERTICAL);
        m_guessPanel->SetSizer(m_guessSizer);
        layout->Add(m_guessPanel, 0, wxALL, 10);
    }

    layout->AddStretchSpacer();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    if (m_onLeave) {
        wxButton* leave = new wxButton(this, wxID_ANY, "Back to map");
        leave->SetFont(Font(12));
        leave->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            Callback cb = m_onLeave;  // the callback may rebuild this page
            if (cb) cb();
        });
        buttons->Add(leave, 0, wxALL, 5);
    }
    buttons->AddStretchSpacer();
    m_continue = AccentButton(this, "Continue", 14);
    m_continue->SetMinSize(wxSize(140, 44));
    m_continue->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        Callback cb = m_onContinue;
        if (cb) cb();
    });
    buttons->Add(m_continue, 0, wxALL, 5);
    layout->Add(buttons, 0, wxEXPAND | wxALL, 5);

    SetSizer(layout);
    RefreshStory();
    Thaw();
    if (m_input) m_input->SetFocus();
}

void StoryScreen::RefreshStory() {
    if (m_storyText) {
        m_storyText->SetLabel(wxString::FromUTF8(m_story->Render(m_solved, "\xC2\xA0")));  // no-break space
        m_storyText->Wrap(kStoryWidth);
        // The wrapped label may be taller now: make its card grow.
        m_storyText->InvalidateBestSize();
        m_storyText->GetParent()->InvalidateBestSize();
        m_storyText->GetParent()->Layout();
    }
    const bool done = m_solved >= m_story->blanks.size();
    // Continue only appears once every blank is solved.
    m_continue->Show(done);
    if (m_inputRow) {
        m_howTo->Show(!done);
        m_guessPanel->Show(!done);
        m_inputRow->Show(!done);
        m_message->SetLabel("");
        m_hint->SetLabel("");
        if (!done) {
            const size_t len = m_story->blanks[m_solved].answer.size();
            m_input->SetMaxLength(static_cast<unsigned long>(len));
            m_input->SetHint(wxString::Format("%zu letters", len));
        }
    }
    Layout();
    FitInside();
    if (done) m_continue->SetFocus();
}

void StoryScreen::AddGuessRow(const std::string& guess, const std::vector<GuessColor>& colors) {
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    for (size_t i = 0; i < guess.size(); ++i) {
        wxStaticText* tile = new wxStaticText(m_guessPanel, wxID_ANY,
            wxString(1, static_cast<wxChar>(std::toupper(static_cast<unsigned char>(guess[i])))),
            wxDefaultPosition, wxSize(36, 36), wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);
        tile->SetFont(Font(16, true));
        tile->SetBackgroundColour(TileColour(colors[i]));
        row->Add(tile, 0, wxALL, 2);
    }
    m_guessSizer->Add(row, 0);
    Layout();
    FitInside();
    Scroll(0, GetVirtualSize().y);  // keep the newest guess and the input in view
}

void StoryScreen::OnGuess() {
    if (!m_story || m_solved >= m_story->blanks.size()) return;
    const StoryBlank& blank = m_story->blanks[m_solved];
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
    if (guess != blank.answer && !m_words->Contains(guess)) {
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
        if (m_input && m_solved < m_story->blanks.size()) m_input->SetFocus();
        return;
    }
    if (++m_wrongOnThisBlank >= kGuessesBeforeHint) {
        const std::string hint = blank.hint.empty() ? DefaultHint(blank.answer) : blank.hint;
        m_hint->SetLabel(wxString::FromUTF8("\xF0\x9F\x92\xA1 Hint: ") + wxString::FromUTF8(hint));  // 💡
        m_hint->Wrap(kTextWidth);
        Layout();
        FitInside();
    }
}
