#include "BibirbleWindow.h"
#include "Theme.h"
#include "SeededRandom.h"
#include "ChristmasVerses.h"
#include "EmbeddedData.h"
#include <wx/iconbndl.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <wx/display.h>
#include <wx/textdlg.h>
#include <algorithm>

namespace {
// Book keys to the tidy names the dropdowns show ("1samuel" -> "1 Samuel").
wxArrayString ToBookNames(const std::vector<std::string>& books) {
    wxArrayString out;
    for (const auto& book : books) out.Add(wxString::FromUTF8(BibleData::BookDisplayName(book)));
    return out;
}
// The app icon for the title bar/taskbar: from Bibirble.rc's "appicon" on
// Windows, or decoded from the PNG compiled into the Linux build.
wxIconBundle LoadAppIcons() {
#ifdef __WXMSW__
    return wxIconBundle("appicon", nullptr);
#else
    wxIconBundle bundle;
    std::string png;
    if (!ReadEmbeddedIconPng(png)) return bundle;
    if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG)) wxImage::AddHandler(new wxPNGHandler);
    wxMemoryInputStream stream(png.data(), png.size());
    wxImage image;
    if (!image.LoadFile(stream, wxBITMAP_TYPE_PNG)) return bundle;
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        wxIcon icon;
        icon.CopyFromBitmap(wxBitmap(image.Scale(size, size, wxIMAGE_QUALITY_HIGH)));
        bundle.AddIcon(icon);
    }
    return bundle;
#endif
}
}  // namespace

wxBEGIN_EVENT_TABLE(BibirbleWindow, wxFrame)
wxEND_EVENT_TABLE()

BibirbleWindow::BibirbleWindow(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Bibirble", wxDefaultPosition, wxSize(500, 850)) {
    // Start in landscape on screens too short for the tall portrait window
    // (most laptops); the game layout follows the window's shape either way.
    {
        const int d = wxDisplay::GetFromWindow(this);
        const wxRect area = wxDisplay(d == wxNOT_FOUND ? 0u : static_cast<unsigned>(d)).GetClientArea();
        if (area.height > 0 && area.height < 900) {
            SetSize(wxSize(std::min(1000, area.width - 40), std::min(680, area.height - 40)));
        }
        SetMinSize(wxSize(420, 420));
        Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
            e.Skip();
            if (m_gameLayout) ApplyGameOrientation();
        });
    }

    m_persistence.Load();
    LoadBibleVersion(m_persistence.GetBibleVersion());
    {
        std::string words;
        if (ReadWordListText(words)) m_words.Load(words);
        for (const auto& answer : m_storyMap.AllAnswers()) m_words.Add(answer);
    }

    const wxIconBundle icons = LoadAppIcons();
    if (!icons.IsEmpty()) SetIcons(icons);

    SetupUi();

    m_settingsScreen->SetBibleVersion(m_data.version().id);
    if (m_persistence.HasLastSeed()) {
        wxString seedStr = wxString::Format("%lld", (long long)m_persistence.GetLastSeed());
        m_settingsScreen->SetSeedText(seedStr);
        m_settingsScreen->SetCurrentSeedDisplay(seedStr);
    }

    ShowStartScreen();
}


void BibirbleWindow::SetupUi() {
    wxColour bgColor = Theme::Background();

    m_centralPanel = new wxPanel(this);
    m_centralPanel->SetBackgroundColour(bgColor);

    m_mainLayout = new wxBoxSizer(wxVERTICAL);

    // Header
    // December: Christmas trees around a red title.
    wxStaticText* title = new wxStaticText(m_centralPanel, wxID_ANY, Theme::Christmas()
        ? wxString::FromUTF8("\xF0\x9F\x8E\x84 Bibirble \xF0\x9F\x8E\x84")  // 🎄 Bibirble 🎄
        : wxString("Bibirble"));
    wxFont titleFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    if (Theme::Christmas()) title->SetForegroundColour(Theme::Accent());
    m_mainLayout->Add(title, 0, wxALL | wxALIGN_CENTER, 10);


    // Start screen
    m_startScreen = new StartScreen(m_centralPanel);
    m_startScreen->SetOnStartDaily([this]() { OnStartDaily(); });
    m_startScreen->SetOnStartRandom([this]() { OnStartRandom(); });
    m_startScreen->SetOnOpenSettings([this]() { OnOpenSettings(); });
    m_startScreen->SetOnOpenStory([this]() { OnOpenStory(); });
    m_mainLayout->Add(m_startScreen, 1, wxEXPAND | wxALL, 5);

    // Settings screen
    m_settingsScreen = new SettingsScreen(m_centralPanel);
    m_settingsScreen->SetOnBack([this]() { OnSettingsBack(); });
    m_settingsScreen->SetOnRandomizeSeed([this]() { OnRandomizeSeed(); });
    m_mainLayout->Add(m_settingsScreen, 1, wxEXPAND | wxALL, 5);

    // Story Mode map
    m_storyMapScreen = new StoryMapScreen(m_centralPanel);
    m_storyMapScreen->SetOnBack([this]() { ShowStartScreen(); });
    m_storyMapScreen->SetOnPlay([this](const std::string& key, bool hard) { OnStoryPlay(key, hard); });
    m_mainLayout->Add(m_storyMapScreen, 1, wxEXPAND | wxALL, 5);

    // Story Mode story pages (with their puzzles)
    m_storyScreen = new StoryScreen(m_centralPanel);
    m_mainLayout->Add(m_storyScreen, 1, wxEXPAND | wxALL, 5);

    // Game panel (reveal text, post-game controls, rows, keyboard)
    SetupGamePanel(m_mainLayout);

    m_centralPanel->SetSizer(m_mainLayout);
}


void BibirbleWindow::SetupGamePanel(wxBoxSizer* parentLayout) {
    m_gamePanel = new wxPanel(m_centralPanel);
    m_gamePanel->SetBackgroundColour(Theme::Background());

    // gameLayout holds the verse/rows column and the number keys: stacked in
    // portrait, side by side in landscape (ApplyGameOrientation).
    m_gameLayout = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gameLayout = new wxBoxSizer(wxVERTICAL);

    // Story Mode bar: level, verse 1-3 of 3, and a way back to the map.
    m_storyHud = new wxPanel(m_gamePanel);
    m_storyHud->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* hudLayout = new wxBoxSizer(wxHORIZONTAL);
    m_storyHudText = new wxStaticText(m_storyHud, wxID_ANY, "");
    m_storyHudText->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    hudLayout->Add(m_storyHudText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 6);
    wxButton* mapBtn = new wxButton(m_storyHud, wxID_ANY, "Story Map");
    mapBtn->SetBackgroundColour(Theme::Accent());
    mapBtn->SetForegroundColour(Theme::AccentText());
    mapBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        SaveStory();  // the attempt (and this verse's guesses) can be continued later
        ShowStoryMap();
    });
    hudLayout->Add(mapBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
    m_storyHud->SetSizer(hudLayout);
    gameLayout->Add(m_storyHud, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    gameLayout->Show(m_storyHud, false);

    // Reveal Panel
    m_revealPanel = new wxStaticText(m_gamePanel, wxID_ANY, "");
    m_revealPanel->SetBackgroundColour(*wxWHITE);
    m_revealPanel->SetMinSize(wxSize(-1, 100));
    m_revealPanel->Wrap(400);
    gameLayout->Add(m_revealPanel, 0, wxALL | wxEXPAND, 10);

    SetupPostGameControls(gameLayout);

    // Scroll Area for Game Rows
    wxScrolledWindow* scrollArea = new wxScrolledWindow(m_gamePanel);
    scrollArea->SetScrollRate(5, 5);
    scrollArea->SetBackgroundColour(Theme::Background());

    m_rowsSizer = new wxBoxSizer(wxVERTICAL);
    scrollArea->SetSizer(m_rowsSizer);

    // 7 Game Rows
    m_allBooks = m_data.getAllBooks();
    wxArrayString books = ToBookNames(m_allBooks);

    for (int i = 0; i < 7; ++i) {
        GameRow* row = new GameRow(scrollArea, books);
        m_rowsSizer->Add(row, 0, wxEXPAND | wxALL, 5);
        m_rows.push_back(row);
    }

    // Track focus on digit inputs so virtual keyboard can restore focus
    m_focusedInput = nullptr;
    for (auto* row : m_rows) {
        auto ctrls = row->GetDigitCtrls();
        for (auto* ctrl : ctrls) {
            if (!ctrl) continue;
            ctrl->Bind(wxEVT_SET_FOCUS, [this, ctrl](wxFocusEvent& ev) {
                m_focusedInput = ctrl;
                ev.Skip();
            });
        }
    }

    // Submit Button
    m_submitBtn = new wxButton(scrollArea, wxID_OK, "Submit Answer");
    m_submitBtn->SetBackgroundColour(Theme::Accent());
    m_submitBtn->SetForegroundColour(Theme::AccentText());
    m_submitBtn->SetMinSize(wxSize(-1, 50));
    wxFont btnFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_submitBtn->SetFont(btnFont);
    m_rowsSizer->Add(m_submitBtn, 0, wxEXPAND | wxALL, 10);

    gameLayout->Add(scrollArea, 1, wxEXPAND);
    m_gameLayout->Add(gameLayout, 1, wxEXPAND);

    // Keyboard
    SetupKeyboard(m_gameLayout);

    m_gamePanel->SetSizer(m_gameLayout);
    parentLayout->Add(m_gamePanel, 1, wxEXPAND | wxALL, 5);
}


void BibirbleWindow::SetupPostGameControls(wxBoxSizer* parentLayout) {
    m_postGameControls = new wxPanel(m_gamePanel);
    wxBoxSizer* layout = new wxBoxSizer(wxHORIZONTAL);

    wxButton* mainMenuBtn = new wxButton(m_postGameControls, wxID_ANY, "Main Menu");
    wxButton* playRandomBtn = new wxButton(m_postGameControls, wxID_ANY, "Play Random");
    wxButton* viewSeedBtn = new wxButton(m_postGameControls, wxID_ANY, "View / Set Seed");

    wxFont postGameBtnFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    for (wxButton* btn : {mainMenuBtn, playRandomBtn, viewSeedBtn}) {
        btn->SetBackgroundColour(Theme::Accent());
        btn->SetForegroundColour(Theme::AccentText());
        btn->SetFont(postGameBtnFont);
    }

    mainMenuBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMainMenu(); });
    playRandomBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnPlayRandom(); });
    viewSeedBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnViewSetSeed(); });

    layout->Add(mainMenuBtn, 1, wxEXPAND | wxALL, 4);
    layout->Add(playRandomBtn, 1, wxEXPAND | wxALL, 4);
    layout->Add(viewSeedBtn, 1, wxEXPAND | wxALL, 4);

    m_postGameControls->SetSizer(layout);
    parentLayout->Add(m_postGameControls, 0, wxEXPAND | wxALL, 5);
    parentLayout->Show(m_postGameControls, false);
}


void BibirbleWindow::SetupKeyboard(wxBoxSizer* parentLayout) {
    wxPanel* kbPanel = new wxPanel(m_gamePanel);
    kbPanel->SetBackgroundColour(Theme::Background());  // keys are solid tiles with gaps, no outlines
    kbPanel->SetMinSize(wxSize(-1, 200));

    wxBoxSizer* hLayout = new wxBoxSizer(wxHORIZONTAL);
    hLayout->SetMinSize(wxSize(-1, 200));

    auto createCol = [&](const wxArrayString& keys) {
        wxBoxSizer* col = new wxBoxSizer(wxVERTICAL);
        for (const auto& k : keys) {
            // No outline: flat keys on the coloured panel.
            wxButton* btn = new wxButton(kbPanel, wxID_ANY, k, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            if (k.Length() == 1 && wxIsdigit(k[0])) m_digitKeys[std::string(k.utf8_str())] = btn;
            btn->SetBackgroundColour(Theme::Accent());
            btn->SetForegroundColour(Theme::AccentText());
            wxFont btnFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
            btn->SetFont(btnFont);
            btn->SetMinSize(wxSize(60, 40));

            btn->Bind(wxEVT_BUTTON, [this, k](wxCommandEvent&) {
                HandleKeyPress(k);
            });

            col->Add(btn, 1, wxEXPAND | wxALL, 3);
        }
        return col;
    };

    wxArrayString col1;
    col1.Add("1"); col1.Add("2"); col1.Add("3");
    hLayout->Add(createCol(col1), 1, wxEXPAND);

    wxArrayString col2;
    col2.Add("4"); col2.Add("5"); col2.Add("6");
    hLayout->Add(createCol(col2), 1, wxEXPAND);

    wxArrayString col3;
    col3.Add("7"); col3.Add("8"); col3.Add("9");
    hLayout->Add(createCol(col3), 1, wxEXPAND);

    wxArrayString col4;
    col4.Add("0"); col4.Add("Del");
    hLayout->Add(createCol(col4), 1, wxEXPAND);

    kbPanel->SetSizer(hLayout);
    parentLayout->Add(kbPanel, 0, wxEXPAND);
    m_keyboardPanel = kbPanel;
}

void BibirbleWindow::ApplyGameOrientation() {
    if (!m_gameLayout || !m_keyboardPanel) return;
    const wxSize client = GetClientSize();
    const bool landscape = client.x > client.y;
    if (landscape == m_landscape && m_gameLayout->GetOrientation() == (landscape ? wxHORIZONTAL : wxVERTICAL)) {
        return;
    }
    m_landscape = landscape;
    m_gameLayout->SetOrientation(landscape ? wxHORIZONTAL : wxVERTICAL);
    // Portrait: a 200px-tall strip under the rows. Landscape: a column on the
    // right, as tall as the window.
    m_keyboardPanel->SetMinSize(landscape ? wxSize(300, -1) : wxSize(-1, 200));
    if (wxSizer* kb = m_keyboardPanel->GetSizer()) kb->SetMinSize(landscape ? wxSize(300, -1) : wxSize(-1, 200));
    m_gamePanel->Layout();
    m_centralPanel->Layout();
}

void BibirbleWindow::UpdateDigitKeyColours() {
    auto rank = [](GuessColor c) { return c == GuessColor::Green ? 2 : c == GuessColor::Yellow ? 1 : 0; };
    std::map<std::string, GuessColor> best;
    for (const auto& record : m_state.history) {
        for (size_t i = 0; i < record.digitGuesses.size() && i < record.digitColors.size(); ++i) {
            auto it = best.find(record.digitGuesses[i]);
            // A key keeps its best clue: green beats yellow beats gray.
            if (it == best.end() || rank(record.digitColors[i]) > rank(it->second)) {
                best[record.digitGuesses[i]] = record.digitColors[i];
            }
        }
    }
    for (auto& [digit, btn] : m_digitKeys) {
        auto it = best.find(digit);
        if (it == best.end()) {
            btn->SetBackgroundColour(Theme::Accent());
            btn->SetForegroundColour(Theme::AccentText());
        } else {
            btn->SetBackgroundColour(Theme::ClueColour(it->second));
            btn->SetForegroundColour(*wxBLACK);
        }
        btn->Refresh();
    }
}


void BibirbleWindow::ShowOnly(wxWindow* screen) {
    for (wxWindow* w : {static_cast<wxWindow*>(m_startScreen), static_cast<wxWindow*>(m_settingsScreen),
                        static_cast<wxWindow*>(m_storyMapScreen), static_cast<wxWindow*>(m_storyScreen),
                        static_cast<wxWindow*>(m_gamePanel)}) {
        m_mainLayout->Show(w, w == screen);
    }
    if (m_storyHud->GetContainingSizer()) {
        m_storyHud->GetContainingSizer()->Show(m_storyHud, m_state.mode == GameMode::Story);
    }
    if (screen == m_gamePanel) ApplyGameOrientation();
    m_gamePanel->Layout();
    m_centralPanel->Layout();
}

void BibirbleWindow::ShowStartScreen() { ShowOnly(m_startScreen); }

void BibirbleWindow::ShowSettingsScreen() { ShowOnly(m_settingsScreen); }

void BibirbleWindow::ShowGamePanel() { ShowOnly(m_gamePanel); }


void BibirbleWindow::OnStartDaily() {
    if (m_persistence.GetLastPlayedDaily() == GetTodayLocalDateString()) {
        wxMessageBox("You already played today! Come back tomorrow.",
                    "Bibirble", wxOK | wxICON_INFORMATION);
        return;
    }
    m_settingsReturnTo = ReturnScreen::Game;
    StartNewGame(GameMode::Daily, m_settingsScreen->GetHardMode(), wxString());
}

void BibirbleWindow::OnStartRandom() {
    m_settingsReturnTo = ReturnScreen::Game;
    StartNewGame(GameMode::Random, m_settingsScreen->GetHardMode(), m_settingsScreen->GetSeedText());
}

void BibirbleWindow::OnOpenSettings() {
    m_settingsReturnTo = ReturnScreen::Menu;
    ShowSettingsScreen();
}

void BibirbleWindow::ApplyBibleVersionSetting() {
    const std::string version = m_settingsScreen->GetBibleVersion();
    if (version == m_data.version().id) return;
    SaveStory();  // keyed by World English Bible references, so it carries over
    LoadBibleVersion(version);
    m_persistence.SetBibleVersion(m_data.version().id);
    m_settingsScreen->SetBibleVersion(m_data.version().id);
}

void BibirbleWindow::OnSettingsBack() {
    // A new Bible version applies to the next game, never the one being
    // played (its verse and book list came from the old version).
    if (m_settingsReturnTo != ReturnScreen::Game) ApplyBibleVersionSetting();
    if (m_settingsReturnTo == ReturnScreen::Game) {
        ShowGamePanel();
    } else {
        ShowStartScreen();
    }
}

void BibirbleWindow::OnRandomizeSeed() {
    int64_t seed = GenerateRandomSeed();
    wxString seedStr = wxString::Format("%lld", (long long)seed);
    m_settingsScreen->SetSeedText(seedStr);
    m_settingsScreen->SetCurrentSeedDisplay(seedStr);
}

void BibirbleWindow::OnMainMenu() {
    ResetGameUi();
    ShowStartScreen();
}

void BibirbleWindow::OnPlayRandom() {
    m_settingsScreen->SetSeedText("");
    StartNewGame(GameMode::Random, m_settingsScreen->GetHardMode(), wxString());
}

void BibirbleWindow::OnViewSetSeed() {
    wxString current = wxString::Format("%lld", (long long)m_state.seed);
    wxTextEntryDialog dlg(this, "View or enter seed (leave blank to cancel):", "Bibirble", current);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString entered = dlg.GetValue();
    if (entered.IsEmpty()) return;

    m_settingsScreen->SetSeedText(entered);
    StartNewGame(GameMode::Random, m_settingsScreen->GetHardMode(), entered);
}


void BibirbleWindow::StartNewGame(GameMode mode, bool hardMode, const wxString& seedText) {
    ApplyBibleVersionSetting();
    if (!m_data.isLoaded()) {
        m_revealPanel->SetLabel("Failed to load data. Please ensure bible_sections.json is in the directory.");
        m_submitBtn->Enable(false);
        ShowGamePanel();
        return;
    }

    int64_t seed;
    if (mode == GameMode::Daily) {
        seed = GetDailySeed();
    } else if (!seedText.IsEmpty()) {
        seed = SeedFromText(std::string(wxString(seedText).Trim().Trim(false).mb_str()));
    } else {
        seed = GenerateRandomSeed();
    }

    // December: the Daily is always a Christmas verse; Random games are too
    // while the Christmas verses setting is on. Matches the web version.
    bool christmas = IsChristmasSeason() &&
        (mode == GameMode::Daily || m_settingsScreen->GetChristmasVerses());
    std::vector<int> candidates;
    if (christmas) {
        for (int i : m_data.getChristmasVerseIndices()) {
            if (!m_data.getVerseAtIndex(i).mature || m_settingsScreen->GetR18Mode()) {
                candidates.push_back(i);
            }
        }
        christmas = !candidates.empty();
    }
    if (!christmas) {
        candidates = m_data.getCandidateIndices(
            m_settingsScreen->GetUsefulOnly(), m_settingsScreen->GetR18Mode());
    }
    int idx;
    if (christmas && mode == GameMode::Daily) {
        int special = -1;
        for (int k = 0; k < (int)candidates.size(); ++k) {
            const Verse v = m_data.getVerseAtIndex(candidates[k]);
            if (v.book == kChristmasDayVerse.book && v.chapter == kChristmasDayVerse.chapter &&
                v.verse == kChristmasDayVerse.verse) {
                special = k;
                break;
            }
        }
        int year, month, day;
        GetTodayLocalDate(year, month, day);
        idx = candidates[PickChristmasDailyPosition((int)candidates.size(), special, year, day)];
    } else {
        idx = candidates[PickIndexFromSeed(seed, (int)candidates.size())];
    }
    // Only offer books that can be the answer in this game's verse pool.
    m_allBooks = m_data.getBooks(candidates);
    Verse verse = m_data.getVerseAtIndex(idx);
    m_state.Reset(mode, seed, hardMode, m_settingsScreen->GetBookHints(), verse, christmas);

    if (mode == GameMode::Random) {
        wxString seedStr = wxString::Format("%lld", (long long)seed);
        m_settingsScreen->SetSeedText(seedStr);
        m_settingsScreen->SetCurrentSeedDisplay(seedStr);
        m_persistence.SetLastSeed(seed);
    } else {
        m_settingsScreen->SetCurrentSeedDisplay("none");
    }

    ResetGameUi();
    UpdateRevealText();
    ShowGamePanel();
}


void BibirbleWindow::ResetGameUi() {
    wxArrayString books = ToBookNames(m_allBooks);
    for (auto* row : m_rows) {
        row->Reset();
        row->SetBooks(books);
    }
    if (!m_rows.empty()) {
        m_rows[0]->setDisabled(false);
    }

    m_submitBtn->SetLabel("Submit Answer");
    m_submitBtn->Enable(true);
    Unbind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);
    Unbind(wxEVT_BUTTON, &BibirbleWindow::OnShare, this, wxID_OK);
    Bind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);

    if (m_postGameControls && m_postGameControls->GetContainingSizer()) {
        m_postGameControls->GetContainingSizer()->Show(m_postGameControls, false);
    }
    UpdateDigitKeyColours();  // history is empty for a new game or verse
    if (m_gamePanel) m_gamePanel->Layout();
}


void BibirbleWindow::UpdateRevealText() {
    // Verse text is UTF-8 (curly quotes, dashes...): convert it explicitly,
    // since passing a std::string to wxString::Format or wxString's
    // constructor uses the C locale and turns such verses into "(null)".
    wxString text;
    if (m_state.gameOver && m_state.currentStage == -1) {
        text = wxString::FromUTF8(m_data.getRevealedText(m_state.targetVerse, 7)) +
               wxString::Format("\n\n- %s %d:%d",
                               wxString::FromUTF8(BibleData::BookDisplayName(m_state.targetVerse.book)),
                               m_state.targetVerse.chapter, m_state.targetVerse.verse);
    } else {
        text = wxString::FromUTF8(m_data.getRevealedText(m_state.targetVerse, m_state.currentStage));
    }
    m_revealPanel->SetLabel(text);
    m_revealPanel->Wrap(400);
}


void BibirbleWindow::HandleKeyPress(const wxString& key) {
    auto isEditableDigit = [](wxTextCtrl* ctrl) {
        return ctrl && ctrl->IsEnabled() && ctrl->IsEditable();
    };

    // Determine target text control
    wxTextCtrl* focused = dynamic_cast<wxTextCtrl*>(m_focusedInput);
    if (focused && !isEditableDigit(focused)) {
        focused = nullptr;
    }

    // If nothing focused, pick the first editable digit in the current active row
    if (!focused) {
        if (m_state.currentStage >= 0 && m_state.currentStage < (int)m_rows.size()) {
            auto ctrls = m_rows[m_state.currentStage]->GetDigitCtrls();
            for (auto* c : ctrls) {
                if (isEditableDigit(c)) { focused = c; break; }
            }
        }
    }

    // If still no control, try to find any enabled digit control
    if (!focused) {
        for (auto* row : m_rows) {
            auto ctrls = row->GetDigitCtrls();
            for (auto* c : ctrls) { if (isEditableDigit(c)) { focused = c; break; } }
            if (focused) break;
        }
    }

    if (!focused) return;

    if (key == "Del") {
        focused->Clear();
        focused->SetFocus();
        return;
    }

    // Only handle single-digit keys
    if (key.Length() == 1 && isdigit(key[0])) {
        focused->SetValue(key);
        // move focus to next digit in the same row if available
        for (auto* row : m_rows) {
            auto ctrls = row->GetDigitCtrls();
            for (size_t i = 0; i < ctrls.size(); ++i) {
                if (ctrls[i] == focused) {
                    if (i + 1 < ctrls.size() && isEditableDigit(ctrls[i + 1])) {
                        ctrls[i+1]->SetFocus();
                        m_focusedInput = ctrls[i+1];
                    } else {
                        focused->SetFocus();
                    }
                    return;
                }
            }
        }
    }
}

void BibirbleWindow::OnVirtualKeyClicked(wxCommandEvent& event) {
    // This will be overridden by button clicks
}

void BibirbleWindow::OnSubmit(wxCommandEvent& event) {
    if (m_state.gameOver) return;

    if (m_state.currentStage >= (int)m_rows.size()) return;

    GameRow* activeRow = m_rows[m_state.currentStage];
    if (!activeRow->isComplete()) {
        wxMessageBox("Please complete all fields in the current row before submitting.",
                    "Bibirble", wxOK | wxICON_WARNING);
        return;
    }

    if (m_state.hardMode) {
        std::string violation = m_state.CheckHardModeViolation(
            activeRow->getBook(), activeRow->getDigits(), m_data);
        if (!violation.empty()) {
            wxMessageBox(violation, "Bibirble", wxOK | wxICON_WARNING);
            return;
        }
    }

    int result = ProcessTurn();
    UpdateDigitKeyColours();
    if (result == -1 && m_state.mode == GameMode::Story) {
        const GuessRecord& last = m_state.history.back();
        const bool won = last.bookColor == GuessColor::Green &&
            std::all_of(last.digitColors.begin(), last.digitColors.end(),
                        [](GuessColor c) { return c == GuessColor::Green; });
        m_state.gameOver = true;
        m_state.currentStage = -1;
        UpdateRevealText();
        HandleStoryVerseEnd(won);
        return;
    }
    if (result == -1) {
        m_state.gameOver = true;
        m_state.currentStage = -1;
        UpdateRevealText();
        m_submitBtn->SetLabel("Share");
        Unbind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);
        Bind(wxEVT_BUTTON, &BibirbleWindow::OnShare, this, wxID_OK);

        if (m_state.mode == GameMode::Daily) {
            m_persistence.SetLastPlayedDaily(GetTodayLocalDateString());
        }

        if (m_postGameControls && m_postGameControls->GetContainingSizer()) {
            m_postGameControls->GetContainingSizer()->Show(m_postGameControls, true);
        }
        m_gamePanel->Layout();
    } else {
        m_state.currentStage = result;
        if (m_state.bookHints) {
            wxArrayString hinted = ToBookNames(m_state.FilterBooksByClues(m_allBooks, m_data));
            for (int i = result; i < (int)m_rows.size(); ++i) {
                m_rows[i]->SetBooks(hinted);
            }
        }
        UpdateRevealText();
        if (m_state.mode == GameMode::Story) {
            // Save every guess so a closed or abandoned verse resumes as it was.
            m_story.Run().currentGuesses = m_state.history;
            SaveStory();
        }
    }
}


int BibirbleWindow::ProcessTurn() {
    GameRow* activeRow = m_rows[m_state.currentStage];

    std::string bookGuess = activeRow->getBook();
    std::vector<std::string> inputs = activeRow->getDigits();

    int correctCount = 0;

    // Check Book
    GuessColor bookColor;
    if (bookGuess == m_state.targetVerse.book) {
        bookColor = GuessColor::Green;
        correctCount++;
    } else if (m_data.getBookArea(bookGuess) == m_state.targetVerse.area) {
        bookColor = GuessColor::Yellow;
    } else {
        bookColor = GuessColor::Gray;
    }
    activeRow->setBookColor(bookColor);

    // Check Digits
    char chStr[3], vStr[3];
    snprintf(chStr, sizeof(chStr), "%02d", m_state.targetVerse.chapter);
    snprintf(vStr, sizeof(vStr), "%02d", m_state.targetVerse.verse);
    std::vector<std::string> answerDigits = {
        std::string(1, chStr[0]), std::string(1, chStr[1]),
        std::string(1, vStr[0]), std::string(1, vStr[1])
    };

    std::vector<GuessColor> results(4, GuessColor::Gray);
    std::vector<std::string> answerPool = answerDigits;

    // Pass 1: Green
    for (int i = 0; i < 4; ++i) {
        if (inputs[i] == answerPool[i]) {
            results[i] = GuessColor::Green;
            answerPool[i] = "";
            correctCount++;
        }
    }

    // Pass 2: Yellow
    for (int i = 0; i < 4; ++i) {
        if (results[i] != GuessColor::Green) {
            for (int j = 0; j < 4; ++j) {
                if (inputs[i] == answerPool[j] && !answerPool[j].empty()) {
                    results[i] = GuessColor::Yellow;
                    answerPool[j] = "";
                    break;
                }
            }
        }
    }

    activeRow->setDigitColors(results);
    activeRow->lockSubmitted();

    GuessRecord record;
    record.bookGuess = bookGuess;
    record.bookColor = bookColor;
    record.digitGuesses = inputs;
    record.digitColors = results;
    m_state.history.push_back(record);

    // Story Mode shows its own messages (HandleStoryVerseEnd).
    const bool story = m_state.mode == GameMode::Story;
    if (correctCount == 5) {
        if (!story) wxMessageBox("You got it correct!", "Winner", wxOK | wxICON_INFORMATION);
        return -1;
    } else if (story && m_state.currentStage + 1 >= (int)m_rows.size()) {
        return -1;
    } else if (m_state.currentStage + 1 >= (int)m_rows.size()) {
        wxString msg = wxString::Format(
            "You ran out of guesses. The correct answer was %s %d:%d. "
            "Maybe you should read your Bible to reflect on what you got wrong!",
            wxString::FromUTF8(BibleData::BookDisplayName(m_state.targetVerse.book)),
            m_state.targetVerse.chapter, m_state.targetVerse.verse);
        wxMessageBox(msg, "Game Over", wxOK | wxICON_INFORMATION);
        return -1;
    }

    // Enable next only after this row is locked with visible colors.
    if (m_state.currentStage + 1 < (int)m_rows.size()) {
        m_rows[m_state.currentStage + 1]->setDisabled(false);
    }

    return m_state.currentStage + 1;
}


void BibirbleWindow::FocusNext() {
    // For future implementation
}

void BibirbleWindow::FocusPrev() {
    // For future implementation
}

void BibirbleWindow::OnShare(wxCommandEvent& event) {
    wxString shareText = wxString::FromUTF8(m_state.BuildShareText(m_data.version().shortName).c_str());

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(shareText));
        wxTheClipboard->Close();
    }

    m_submitBtn->SetLabel("Copied to Clipboard");
}


// ---------------------------------------------------------------- Story Mode

void BibirbleWindow::LoadBibleVersion(const std::string& id) {
    const BibleVersion& wanted = FindBibleVersion(id);
    if (!m_data.loadVersion(wanted) && &wanted != &BibleVersions().front()) {
        m_data.loadVersion(BibleVersions().front());  // fall back to the World English Bible
    }
    if (!m_data.isLoaded()) return;
    // Story pools are resolved against the loaded verses, so they're rebuilt
    // for each version; progress is keyed by World English Bible references.
    m_storyMap.Load(m_data);
    m_story.FromJson(m_persistence.GetStory(), m_data);
    if (m_storyMap.IsLoaded()) {
        // Drop a saved attempt whose level no longer exists in the map.
        if (m_story.Run().active && !m_storyMap.FindLevel(m_story.Run().levelKey)) m_story.ClearRun();
        m_story.UpdateUnlocks(m_storyMap);
    }
}

bool BibirbleWindow::AllowMature() const {
    return m_settingsScreen->GetR18Mode();
}

void BibirbleWindow::SaveStory() {
    if (m_data.isLoaded()) m_persistence.SetStory(m_story.ToJson(m_data));
}

void BibirbleWindow::OnOpenStory() {
    ApplyBibleVersionSetting();
    if (!m_data.isLoaded()) {
        wxMessageBox("Story Mode needs the Bible data, which didn't load.", "Bibirble", wxOK | wxICON_WARNING);
        return;
    }
    ShowStoryMap();
}

void BibirbleWindow::ShowStoryMap() {
    if (m_storyMap.IsLoaded()) m_story.UpdateUnlocks(m_storyMap);
    m_storyMapScreen->ShowMap(m_storyMap, m_story, m_data, AllowMature());
    ShowOnly(m_storyMapScreen);
}

void BibirbleWindow::OnStoryPlay(const std::string& levelKey, bool hardMode) {
    const StoryLevel* level = m_storyMap.FindLevel(levelKey);
    if (!level || !m_story.IsUnlocked(levelKey)) return;

    const StoryRun& run = m_story.Run();
    const bool resume = run.active && run.levelKey == levelKey;
    if (!resume) {
        // Hard Mode only once this level has 4+ stars.
        const bool hard = hardMode && level->allowHard && m_story.HardModeUnlocked(levelKey);
        const LevelProgress* lp = m_story.Get(levelKey);
        const bool retryAfterFail = lp && lp->failedVerse >= 0;
        m_story.StartRun(*level, m_data, AllowMature(), hard, m_rng);
        // The intro was already read (and its puzzles solved) on the attempt
        // that just failed.
        if (retryAfterFail) m_story.Run().storyDone = true;
        SaveStory();
    }
    ContinueStoryRun();
}

// Shows the story due before the next verse (the intro, or the "between"
// text after verse 1 or 2) as a full screen -- its blanks must be solved to
// go on -- then starts that verse. "Back to map" leaves the attempt saved
// with the story still to do.
void BibirbleWindow::ContinueStoryRun() {
    StoryRun& run = m_story.Run();
    const StoryLevel* level = m_storyMap.FindLevel(run.levelKey);
    if (!run.active || !level) return;
    const StoryText* text = run.storyDone ? nullptr : level->StoryBefore(run.solvedCount);
    if (!text || text->Empty()) {
        run.storyDone = true;
        SaveStory();
        StartStoryVerse();
        return;
    }
    m_storyScreen->ShowStory(wxString::FromUTF8(level->title), wxString(), *text, m_words, run.hardMode,
        [this]() {
            m_story.Run().storyDone = true;
            SaveStory();
            StartStoryVerse();
        },
        [this]() {
            SaveStory();
            ShowStoryMap();
        });
    ShowOnly(m_storyScreen);
}

void BibirbleWindow::StartStoryVerse() {
    const StoryRun& run = m_story.Run();
    if (!run.active) return;
    const Verse verse = m_data.getVerseAtIndex(run.verses[run.solvedCount]);

    // Every book is offered: limiting the list to the level's pool would give
    // the answer away.
    m_allBooks = m_data.getAllBooks();
    m_state.Reset(GameMode::Story, 0, run.hardMode, m_settingsScreen->GetBookHints(), verse);
    m_settingsScreen->SetCurrentSeedDisplay("none");
    ResetGameUi();

    // Put back guesses already made on this verse (after closing the app or
    // going back to the map).
    const std::vector<GuessRecord> saved = run.currentGuesses;
    for (size_t i = 0; i < saved.size() && i + 1 < m_rows.size(); ++i) {
        GameRow* row = m_rows[i];
        row->setDisabled(false);
        row->SetGuess(saved[i].bookGuess, saved[i].digitGuesses);
        row->setBookColor(saved[i].bookColor);
        row->setDigitColors(saved[i].digitColors);
        row->lockSubmitted();
        m_state.history.push_back(saved[i]);
        m_state.currentStage = static_cast<int>(i) + 1;
    }
    if (m_state.currentStage > 0) {
        m_rows[m_state.currentStage]->setDisabled(false);
        if (m_state.bookHints) {
            wxArrayString hinted = ToBookNames(m_state.FilterBooksByClues(m_allBooks, m_data));
            for (int i = m_state.currentStage; i < (int)m_rows.size(); ++i) m_rows[i]->SetBooks(hinted);
        }
    }

    UpdateDigitKeyColours();
    UpdateRevealText();
    UpdateStoryHud();
    ShowGamePanel();
}

void BibirbleWindow::UpdateStoryHud() {
    const StoryRun& run = m_story.Run();
    const StoryLevel* level = m_storyMap.FindLevel(run.levelKey);
    if (!level) return;
    const wxString dot = wxString::FromUTF8("  \xC2\xB7  ");  // ·
    wxString text = wxString::FromUTF8("\xF0\x9F\x93\x96 ") + wxString::FromUTF8(level->title) + dot +  // 📖
        wxString::Format("Verse %d of %d", run.solvedCount + 1, StoryProgress::kVersesPerLevel);
    if (run.hardMode) text += dot + "Hard Mode";
    m_storyHudText->SetLabel(text);
    m_storyHud->Layout();
}

void BibirbleWindow::HandleStoryVerseEnd(bool won) {
    const std::string levelKey = m_story.Run().levelKey;
    const StoryLevel* level = m_storyMap.FindLevel(levelKey);
    const wxString answer = wxString::Format("%s %d:%d",
        wxString::FromUTF8(BibleData::BookDisplayName(m_state.targetVerse.book)),
        m_state.targetVerse.chapter, m_state.targetVerse.verse);

    if (!won) {
        const bool hard = m_story.Run().hardMode;
        m_story.RecordFail();
        SaveStory();
        const int choice = wxMessageBox(
            "Out of guesses! The answer was " + answer + ".\n\n"
            "Retry the level? This verse will come back so you can try it again.",
            "Level failed", wxYES_NO | wxICON_INFORMATION, this);
        if (choice == wxYES) {
            OnStoryPlay(levelKey, hard);
        } else {
            ShowStoryMap();
        }
        return;
    }

    const int guesses = static_cast<int>(m_state.history.size());
    const bool finished = m_story.RecordVerseSolved(guesses);
    if (!finished) {
        SaveStory();
        wxMessageBox(wxString::Format("%s in %d %s! On to verse %d of %d.", answer, guesses,
                                      guesses == 1 ? "guess" : "guesses",
                                      m_story.Run().solvedCount + 1, StoryProgress::kVersesPerLevel),
                     "Verse solved", wxOK | wxICON_INFORMATION, this);
        ContinueStoryRun();
        return;
    }

    const bool passedInHardMode = m_story.Run().hardMode;  // RecordPass clears the run
    const PassResult r = m_story.RecordPass(m_storyMap, m_data, AllowMature());
    SaveStory();

    const wxString star = wxString::FromUTF8("\xE2\x98\x85");  // ★
    auto line = [&](bool got, const wxString& what) {
        return (got ? star : wxString("-")) + " " + what + "\n";
    };
    wxString msg = wxString::Format("Level complete!  %s\n\n", StarString(r.earned.Total(), level ? level->MaxStars() : 7));
    msg += wxString::Format("%s Guesses: %d of 3\n", star, r.earned.guess);
    if (level && level->allowHard) msg += line(r.earned.hard, "Hard Mode");
    if (level && level->allowFirstTry) msg += line(r.earned.firstTry, "A verse on the first try");
    if (level && level->allowNoRetry) msg += line(r.earned.noRetry, "No retry");
    if (level && level->allowPerfect) {
        msg += line(r.earned.perfect, wxString::Format("Perfect (pool %d / %d solved)", r.perfectSolved, r.perfectTotal));
    }
    msg += wxString::Format("\nBest on this level: %s", StarString(r.best.Total(), level ? level->MaxStars() : 7));
    if (r.hardModeUnlocked) msg += "\n\nHard Mode unlocked for this level!";
    int newBonus = 0;
    for (const auto& key : r.newlyUnlocked) {
        const StoryLevel* l = m_storyMap.FindLevel(key);
        if (l && l->isBonus) ++newBonus;
    }
    if (newBonus > 0) msg += wxString::FromUTF8("\n\n\xF0\x9F\x94\xAE A hidden prophecy level has appeared!");  // 🔮

    if (!level) {
        ShowStoryMap();
        return;
    }
    // Results and the rest of the story (with any puzzles) on one full
    // screen. Leaving it just goes to the map.
    m_storyScreen->ShowStory(wxString::FromUTF8(level->title), msg, level->outro, m_words, passedInHardMode,
                             [this]() { ShowStoryMap(); }, [this]() { ShowStoryMap(); });
    ShowOnly(m_storyScreen);
}
