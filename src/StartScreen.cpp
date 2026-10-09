#include "StartScreen.h"

#include <ctime>

namespace {
wxString FormatCountdownToNextUtcMidnight() {
    std::time_t t = std::time(nullptr);
    std::tm utcTm{};
#if defined(_WIN32)
    gmtime_s(&utcTm, &t);
#else
    gmtime_r(&t, &utcTm);
#endif
    int secondsIntoDay = utcTm.tm_hour * 3600 + utcTm.tm_min * 60 + utcTm.tm_sec;
    int secondsLeft = 24 * 3600 - secondsIntoDay;
    if (secondsLeft < 0) secondsLeft = 0;

    int hrs = secondsLeft / 3600;
    int mins = (secondsLeft % 3600) / 60;
    int secs = secondsLeft % 60;
    return wxString::Format("Next Challenge arrives in: %02d:%02d:%02d", hrs, mins, secs);
}
}  // namespace

StartScreen::StartScreen(wxWindow* parent) : wxPanel(parent), m_timer(this) {
    SetupUi();

    Bind(wxEVT_TIMER, &StartScreen::UpdateCountdown, this, m_timer.GetId());
    m_timer.Start(1000);
    if (m_countdownText) {
        m_countdownText->SetLabel(FormatCountdownToNextUtcMidnight());
    }
}

void StartScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* headline = new wxStaticText(this, wxID_ANY, "Choose a mode to begin");
    wxFont headlineFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    headline->SetFont(headlineFont);
    layout->Add(headline, 0, wxALL | wxALIGN_CENTER, 10);

    wxBoxSizer* actions = new wxBoxSizer(wxHORIZONTAL);
    auto makeButton = [&](const wxString& label) {
        wxButton* btn = new wxButton(this, wxID_ANY, label);
        btn->SetBackgroundColour(wxColour(200, 100, 50));
        btn->SetForegroundColour(wxColour(235, 230, 157));
        wxFont btnFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        btn->SetFont(btnFont);
        btn->SetMinSize(wxSize(-1, 40));
        return btn;
    };

    wxButton* startDailyBtn = makeButton("Start Daily");
    wxButton* startRandomBtn = makeButton("Start Random");
    wxButton* settingsBtn = makeButton("Settings");

    startDailyBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onStartDaily) m_onStartDaily();
    });
    startRandomBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onStartRandom) m_onStartRandom();
    });
    settingsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onOpenSettings) m_onOpenSettings();
    });

    actions->Add(startDailyBtn, 1, wxEXPAND | wxALL, 5);
    actions->Add(startRandomBtn, 1, wxEXPAND | wxALL, 5);
    actions->Add(settingsBtn, 1, wxEXPAND | wxALL, 5);
    layout->Add(actions, 0, wxEXPAND | wxALL, 10);

    wxPanel* newsCard = new wxPanel(this);
    newsCard->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* newsLayout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* newsTitle = new wxStaticText(newsCard, wxID_ANY, "What's New");
    wxFont newsFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    newsTitle->SetFont(newsFont);
    newsLayout->Add(newsTitle, 0, wxALL, 8);

    const char* announcements[] = {
        "Book hints: an accessibility option in Settings. After each guess, "
        "the next book list only shows books that still fit your clues. Not "
        "available in hard mode.",
        "Bible order: books in the dropdown now go Genesis to Revelation.",
        "Useful verses only now has 365 verses to play.",
        "Useful verses only: turn it on in Settings to play only well-known "
        "verses like John 3:16 and Philippians 4:13. No more genealogies!",
        "Poetry is here: Psalms, Proverbs, Job, Lamentations and Song of "
        "Solomon verses are now in the game. Psalm 23 included!",
        "Fixed verses: verses that run across two paragraphs now show the "
        "whole verse instead of only the second half.",
        "Psalms past chapter 99 aren't included yet, since guesses use two "
        "chapter digits.",
    };
    for (const char* text : announcements) {
        wxStaticText* item = new wxStaticText(newsCard, wxID_ANY, wxString::FromUTF8("\xE2\x80\xA2 ") + text);
        item->Wrap(400);
        newsLayout->Add(item, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    }

    newsCard->SetSizer(newsLayout);
    layout->Add(newsCard, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    wxPanel* infoCard = new wxPanel(this);
    infoCard->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* infoLayout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* overviewTitle = new wxStaticText(infoCard, wxID_ANY, "Game Overview");
    wxFont overviewFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    overviewTitle->SetFont(overviewFont);
    infoLayout->Add(overviewTitle, 0, wxALL, 8);

    wxStaticText* overviewText = new wxStaticText(infoCard, wxID_ANY,
        "Guess the verse by selecting the correct book and entering the "
        "chapter:verse digits. Green means exact match, yellow means correct "
        "digit in the wrong place, gray means not in the answer.");
    overviewText->Wrap(400);
    infoLayout->Add(overviewText, 0, wxALL, 8);

    wxStaticText* howToPlay = new wxStaticText(infoCard, wxID_ANY,
        "How to play: Choose a book, then enter the four chapter/verse "
        "digits based on the clue.");
    howToPlay->Wrap(400);
    infoLayout->Add(howToPlay, 0, wxALL, 8);

    wxStaticText* seedHint = new wxStaticText(infoCard, wxID_ANY,
        "Seed hint: Use the Settings screen to set or randomize a seed for "
        "replayable games.");
    seedHint->Wrap(400);
    infoLayout->Add(seedHint, 0, wxALL, 8);

    m_countdownText = new wxStaticText(infoCard, wxID_ANY, "");
    infoLayout->Add(m_countdownText, 0, wxALL, 8);

    infoCard->SetSizer(infoLayout);
    layout->Add(infoCard, 0, wxEXPAND | wxALL, 10);

    SetSizer(layout);
}

void StartScreen::UpdateCountdown(wxTimerEvent& event) {
    if (m_countdownText) {
        m_countdownText->SetLabel(FormatCountdownToNextUtcMidnight());
    }
}
