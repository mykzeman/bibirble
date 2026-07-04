#include "BibirbleWindow.h"
#include "SeededRandom.h"
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <wx/textdlg.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(BibirbleWindow, wxFrame)
wxEND_EVENT_TABLE()

BibirbleWindow::BibirbleWindow(wxWindow* parent, const std::string& dataPath)
    : wxFrame(parent, wxID_ANY, "Bibirble", wxDefaultPosition, wxSize(500, 850)) {

    const std::string resolvedPath = dataPath.empty()
        ? BibleData::ResolveDataFilePath("bible_sections.json")
        : dataPath;
    m_data.loadData(resolvedPath);

    m_persistence.Load();

    SetupUi();

    if (m_persistence.HasLastSeed()) {
        wxString seedStr = wxString::Format("%lld", (long long)m_persistence.GetLastSeed());
        m_settingsScreen->SetSeedText(seedStr);
        m_settingsScreen->SetCurrentSeedDisplay(seedStr);
    }

    ShowStartScreen();
}


void BibirbleWindow::SetupUi() {
    wxColour bgColor(234, 229, 159);  // #eae59f

    m_centralPanel = new wxPanel(this);
    m_centralPanel->SetBackgroundColour(bgColor);

    m_mainLayout = new wxBoxSizer(wxVERTICAL);
    m_mainLayout->SetMinSize(wxSize(500, 850));

    // Header
    wxStaticText* title = new wxStaticText(m_centralPanel, wxID_ANY, "Bibirble");
    wxFont titleFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    m_mainLayout->Add(title, 0, wxALL | wxALIGN_CENTER, 10);

    wxStaticText* subtitle = new wxStaticText(m_centralPanel, wxID_ANY, "World English Bible Version");
    m_mainLayout->Add(subtitle, 0, wxALL | wxALIGN_CENTER, 5);

    // Start screen
    m_startScreen = new StartScreen(m_centralPanel);
    m_startScreen->SetOnStartDaily([this]() { OnStartDaily(); });
    m_startScreen->SetOnStartRandom([this]() { OnStartRandom(); });
    m_startScreen->SetOnOpenSettings([this]() { OnOpenSettings(); });
    m_mainLayout->Add(m_startScreen, 1, wxEXPAND | wxALL, 5);

    // Settings screen
    m_settingsScreen = new SettingsScreen(m_centralPanel);
    m_settingsScreen->SetOnBack([this]() { OnSettingsBack(); });
    m_settingsScreen->SetOnRandomizeSeed([this]() { OnRandomizeSeed(); });
    m_mainLayout->Add(m_settingsScreen, 1, wxEXPAND | wxALL, 5);

    // Game panel (reveal text, post-game controls, rows, keyboard)
    SetupGamePanel(m_mainLayout);

    m_centralPanel->SetSizer(m_mainLayout);
}


void BibirbleWindow::SetupGamePanel(wxBoxSizer* parentLayout) {
    m_gamePanel = new wxPanel(m_centralPanel);
    m_gamePanel->SetBackgroundColour(wxColour(234, 229, 159));

    wxBoxSizer* gameLayout = new wxBoxSizer(wxVERTICAL);

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
    scrollArea->SetBackgroundColour(wxColour(234, 229, 159));

    m_rowsSizer = new wxBoxSizer(wxVERTICAL);
    scrollArea->SetSizer(m_rowsSizer);

    // 7 Game Rows
    wxArrayString books;
    std::vector<std::string> bookList = m_data.getAllBooks();
    std::sort(bookList.begin(), bookList.end());
    for (const auto& book : bookList) {
        books.Add(book);
    }

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
    m_submitBtn->SetBackgroundColour(wxColour(200, 100, 50));
    m_submitBtn->SetForegroundColour(wxColour(235, 230, 157));
    m_submitBtn->SetMinSize(wxSize(-1, 50));
    wxFont btnFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_submitBtn->SetFont(btnFont);
    m_rowsSizer->Add(m_submitBtn, 0, wxEXPAND | wxALL, 10);

    gameLayout->Add(scrollArea, 1, wxEXPAND);

    // Keyboard
    SetupKeyboard(gameLayout);

    m_gamePanel->SetSizer(gameLayout);
    parentLayout->Add(m_gamePanel, 1, wxEXPAND | wxALL, 5);
}


void BibirbleWindow::SetupPostGameControls(wxBoxSizer* parentLayout) {
    m_postGameControls = new wxPanel(m_gamePanel);
    wxBoxSizer* layout = new wxBoxSizer(wxHORIZONTAL);

    wxButton* mainMenuBtn = new wxButton(m_postGameControls, wxID_ANY, "Main Menu");
    wxButton* playRandomBtn = new wxButton(m_postGameControls, wxID_ANY, "Play Random");
    wxButton* viewSeedBtn = new wxButton(m_postGameControls, wxID_ANY, "View / Set Seed");

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
    kbPanel->SetBackgroundColour(wxColour(200, 100, 50));
    kbPanel->SetMinSize(wxSize(-1, 200));

    wxBoxSizer* hLayout = new wxBoxSizer(wxHORIZONTAL);
    hLayout->SetMinSize(wxSize(-1, 200));

    auto createCol = [&](const wxArrayString& keys) {
        wxBoxSizer* col = new wxBoxSizer(wxVERTICAL);
        for (const auto& k : keys) {
            wxButton* btn = new wxButton(kbPanel, wxID_ANY, k);
            btn->SetBackgroundColour(wxColour(200, 100, 50));
            btn->SetForegroundColour(wxColour(235, 230, 157));
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
}


void BibirbleWindow::ShowStartScreen() {
    m_mainLayout->Show(m_startScreen, true);
    m_mainLayout->Show(m_settingsScreen, false);
    m_mainLayout->Show(m_gamePanel, false);
    m_centralPanel->Layout();
}

void BibirbleWindow::ShowSettingsScreen() {
    m_mainLayout->Show(m_startScreen, false);
    m_mainLayout->Show(m_settingsScreen, true);
    m_mainLayout->Show(m_gamePanel, false);
    m_centralPanel->Layout();
}

void BibirbleWindow::ShowGamePanel() {
    m_mainLayout->Show(m_startScreen, false);
    m_mainLayout->Show(m_settingsScreen, false);
    m_mainLayout->Show(m_gamePanel, true);
    m_centralPanel->Layout();
}


void BibirbleWindow::OnStartDaily() {
    if (m_persistence.GetLastPlayedDaily() == GetTodayUtcDateString()) {
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

void BibirbleWindow::OnSettingsBack() {
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
        seed = HashStringToInt(std::string(seedText.mb_str()));
    } else {
        seed = GenerateRandomSeed();
    }

    int idx = PickIndexFromSeed(seed, m_data.getVerseCount());
    Verse verse = m_data.getVerseAtIndex(idx);
    m_state.Reset(mode, seed, hardMode, verse);

    wxLogMessage("Target: %s %d:%d (seed=%lld)", verse.book, verse.chapter, verse.verse, (long long)seed);

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
    for (auto* row : m_rows) {
        row->Reset();
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
    if (m_gamePanel) m_gamePanel->Layout();
}


void BibirbleWindow::UpdateRevealText() {
    wxString text;
    if (m_state.gameOver && m_state.currentStage == -1) {
        text = wxString::Format("%s\n\n- %s %d:%d",
                               m_data.getRevealedText(m_state.targetVerse, 7),
                               m_state.targetVerse.book, m_state.targetVerse.chapter, m_state.targetVerse.verse);
    } else {
        text = m_data.getRevealedText(m_state.targetVerse, m_state.currentStage);
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
    if (result == -1) {
        m_state.gameOver = true;
        m_state.currentStage = -1;
        UpdateRevealText();
        m_submitBtn->SetLabel("Share");
        Unbind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);
        Bind(wxEVT_BUTTON, &BibirbleWindow::OnShare, this, wxID_OK);

        if (m_state.mode == GameMode::Daily) {
            m_persistence.SetLastPlayedDaily(GetTodayUtcDateString());
        }

        if (m_postGameControls && m_postGameControls->GetContainingSizer()) {
            m_postGameControls->GetContainingSizer()->Show(m_postGameControls, true);
        }
        m_gamePanel->Layout();
    } else {
        m_state.currentStage = result;
        UpdateRevealText();
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

    if (correctCount == 5) {
        wxMessageBox("You got it correct!", "Winner", wxOK | wxICON_INFORMATION);
        return -1;
    } else if (m_state.currentStage + 1 >= (int)m_rows.size()) {
        wxString msg = wxString::Format(
            "You ran out of guesses. The correct answer was %s %d:%d. "
            "Maybe you should read your Bible to reflect on what you got wrong!",
            m_state.targetVerse.book, m_state.targetVerse.chapter, m_state.targetVerse.verse);
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
    wxString shareText = wxString::FromUTF8(m_state.BuildShareText().c_str());

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(shareText));
        wxTheClipboard->Close();
    }

    m_submitBtn->SetLabel("Copied to Clipboard");
}
