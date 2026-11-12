#include "StartScreen.h"
#include "Theme.h"

#include <ctime>

#include "SeededRandom.h"

namespace {
// Time left until the next Daily, which arrives at the player's local
// midnight (the same day GetTodayLocalDateString uses). Like the web's
// new Date(y, m, d + 1) - now: mktime rolls day 32 over into the next month
// and gets 23- and 25-hour daylight-saving days right.
wxString FormatCountdownToNextLocalMidnight() {
    std::time_t now = std::time(nullptr);
    std::tm nextMidnight{};
#if defined(_WIN32)
    localtime_s(&nextMidnight, &now);
#else
    localtime_r(&now, &nextMidnight);
#endif
    nextMidnight.tm_mday += 1;
    nextMidnight.tm_hour = 0;
    nextMidnight.tm_min = 0;
    nextMidnight.tm_sec = 0;
    nextMidnight.tm_isdst = -1;  // let mktime work out daylight saving
    int secondsLeft = static_cast<int>(std::difftime(std::mktime(&nextMidnight), now));
    if (secondsLeft < 0) secondsLeft = 0;

    int hrs = secondsLeft / 3600;
    int mins = (secondsLeft % 3600) / 60;
    int secs = secondsLeft % 60;
    return wxString::Format("Next Challenge arrives in: %02d:%02d:%02d", hrs, mins, secs);
}
}  // namespace

StartScreen::StartScreen(wxWindow* parent) : wxScrolledWindow(parent), m_timer(this) {
    SetScrollRate(0, 10);
    SetupUi();

    Bind(wxEVT_TIMER, &StartScreen::UpdateCountdown, this, m_timer.GetId());
    m_timer.Start(1000);
    if (m_countdownText) {
        m_countdownText->SetLabel(FormatCountdownToNextLocalMidnight());
    }
}

void StartScreen::SetupUi() {
    wxColour bgColor = Theme::Background();
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    // December: a Merry Christmas banner, like the web version.
    if (Theme::Christmas()) {
        int year, month, day;
        GetTodayLocalDate(year, month, day);
        wxPanel* banner = new wxPanel(this);
        banner->SetBackgroundColour(*wxWHITE);
        wxBoxSizer* bannerLayout = new wxBoxSizer(wxVERTICAL);
        wxStaticText* bannerText = new wxStaticText(banner, wxID_ANY, wxString::FromUTF8(day == 25
            ? "\xF0\x9F\x8E\x81 Merry Christmas! Jesus is born! Today's Daily is extra special."
            : "\xF0\x9F\x8E\x84 Merry Christmas! Every Daily this month is a Christmas verse."),
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        bannerText->Wrap(400);
        bannerLayout->Add(bannerText, 0, wxALL | wxALIGN_CENTER, 10);
        banner->SetSizer(bannerLayout);
        layout->Add(banner, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    }

    wxStaticText* headline = new wxStaticText(this, wxID_ANY, "Choose a mode to begin");
    wxFont headlineFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    headline->SetFont(headlineFont);
    layout->Add(headline, 0, wxALL | wxALIGN_CENTER, 10);

    wxBoxSizer* actions = new wxBoxSizer(wxHORIZONTAL);
    auto makeButton = [&](const wxString& label) {
        wxButton* btn = new wxButton(this, wxID_ANY, label);
        btn->SetBackgroundColour(Theme::Accent());
        btn->SetForegroundColour(Theme::AccentText());
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

    auto addHeading = [&](const char* text) {
        wxStaticText* heading = new wxStaticText(newsCard, wxID_ANY, text);
        wxFont font = heading->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        heading->SetFont(font);
        newsLayout->Add(heading, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    };
    auto addItems = [&](std::initializer_list<const char*> items) {
        for (const char* text : items) {
            wxStaticText* item = new wxStaticText(newsCard, wxID_ANY, wxString::FromUTF8("\xE2\x80\xA2 ") + text);
            item->Wrap(400);
            newsLayout->Add(item, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
        }
    };

    addHeading("New in this update");
    addItems({
        "Daily reset: a new Daily arrives at midnight your time, the same "
        "as the web version.",
        "Christmas: all through December the app gets festive colours and "
        "every Daily is a Christmas verse, with Luke 2:11 on Christmas Day. "
        "Random games can pick Christmas verses too (a December setting).",
        "Book names: the book dropdown shows tidy names like 1 Thessalonians "
        "and Song of Solomon.",
    });
    addHeading("Earlier");
    addItems({
        "Daily fix: the daily verse now jumps all over the Bible instead of "
        "moving one or two verses a day. Seeds typed as numbers now match the "
        "web version.",
        "1200 useful verses: Useful verses only now has 1200 verses, up from "
        "365, ranked so verses about God, faith and life come first.",
        "Book list: the book dropdown only shows books that have a verse in "
        "the pool you're playing (so Useful mode skips books like Numbers).",
        "R18 mode: verses with mature content are now hidden by default. "
        "Players 18 or older can turn on R18 mode in Settings after picking "
        "their date of birth on a calendar.",
        "Book hints: an accessibility option in Settings. After each guess, "
        "the next book list only shows books that still fit your clues. Not "
        "available in hard mode.",
        "Bible order: books in the dropdown now go Genesis to Revelation.",
        "More useful verses: Useful verses only now has 365 family-friendly "
        "verses, picked to skip genealogies, census lists and measurements.",
        "Useful verses only: play only well-known verses like John 3:16 and "
        "Philippians 4:13.",
        "Poetry is here: Psalms, Proverbs, Job, Lamentations and Song of "
        "Solomon. Psalm 23 included! (Psalms past chapter 99 aren't included "
        "yet, since guesses use two chapter digits.)",
        "Fixed verses: verses that run across two paragraphs now show in full.",
    });

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
        m_countdownText->SetLabel(FormatCountdownToNextLocalMidnight());
    }
}
