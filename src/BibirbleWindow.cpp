#include "BibirbleWindow.h"
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(BibirbleWindow, wxFrame)
    EVT_BUTTON(wxID_OK, BibirbleWindow::OnSubmit)
wxEND_EVENT_TABLE()

BibirbleWindow::BibirbleWindow(wxWindow* parent, const std::string& dataPath)
    : wxFrame(parent, wxID_ANY, "Bibirble", wxDefaultPosition, wxSize(500, 850)) {

    const std::string resolvedPath = dataPath.empty()
        ? BibleData::ResolveDataFilePath("bible_sections.json")
        : dataPath;
    m_data.loadData(resolvedPath);
    SetupUi();
    StartNewGame();
}


void BibirbleWindow::SetupUi() {
    wxColour bgColor(234, 229, 159);  // #eae59f
    
    m_centralPanel = new wxPanel(this);
    m_centralPanel->SetBackgroundColour(bgColor);
    
    wxBoxSizer* mainLayout = new wxBoxSizer(wxVERTICAL);
    mainLayout->SetMinSize(wxSize(500, 850));
    
    // Header
    wxStaticText* title = new wxStaticText(m_centralPanel, wxID_ANY, "Bibirble");
    wxFont titleFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    mainLayout->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    
    wxStaticText* subtitle = new wxStaticText(m_centralPanel, wxID_ANY, "World English Bible Version");
    mainLayout->Add(subtitle, 0, wxALL | wxALIGN_CENTER, 5);
    
    // Reveal Panel
    m_revealPanel = new wxStaticText(m_centralPanel, wxID_ANY, "Loading...");
    m_revealPanel->SetBackgroundColour(*wxWHITE);
    m_revealPanel->SetMinSize(wxSize(-1, 100));
    m_revealPanel->Wrap(400);
    mainLayout->Add(m_revealPanel, 0, wxALL | wxEXPAND, 10);
    
    // Scroll Area for Game Rows
    wxScrolledWindow* scrollArea = new wxScrolledWindow(m_centralPanel);
    scrollArea->SetScrollRate(5, 5);
    scrollArea->SetBackgroundColour(bgColor);
    
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
    Bind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);
    m_rowsSizer->Add(m_submitBtn, 0, wxEXPAND | wxALL, 10);
    
    mainLayout->Add(scrollArea, 1, wxEXPAND);
    
    // Keyboard
    SetupKeyboard(mainLayout);
    
    m_centralPanel->SetSizer(mainLayout);
}


void BibirbleWindow::SetupKeyboard(wxBoxSizer* parentLayout) {
    wxPanel* kbPanel = new wxPanel(m_centralPanel);
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
            
            Bind(wxEVT_BUTTON, [this, k](wxCommandEvent&) {
                HandleKeyPress(k);
            }, btn->GetId());
            
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


void BibirbleWindow::StartNewGame() {
    if (!m_data.isLoaded()) {
        m_revealPanel->SetLabel("Failed to load data. Please ensure bible_sections.json is in the directory.");
        m_submitBtn->Enable(false);
        return;
    }
    
    Verse verse = m_data.getRandomVerse();
    m_state.Reset(GameMode::Random, 0, false, verse);
    wxLogMessage("Target: %s %d:%d", m_state.targetVerse.book, m_state.targetVerse.chapter, m_state.targetVerse.verse);

    m_submitBtn->SetLabel("Submit Answer");
    m_submitBtn->Enable(true);
    
    for (int i = 0; i < (int)m_rows.size(); ++i) {
        m_rows[i]->setDisabled(true);
    }
    if (!m_rows.empty()) {
        m_rows[0]->setDisabled(false);
    }
    
    UpdateRevealText();
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

    int result = ProcessTurn();
    if (result == -1) {
        m_state.gameOver = true;
        m_state.currentStage = -1;
        UpdateRevealText();
        m_submitBtn->SetLabel("Share");
        Unbind(wxEVT_BUTTON, &BibirbleWindow::OnSubmit, this, wxID_OK);
        Bind(wxEVT_BUTTON, &BibirbleWindow::OnShare, this, wxID_OK);
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
    wxString shareText = wxString::Format(
        "Could you beat this score in Bibirble?\n\n- %s %d:%d\n\n(Result grid copied to clipboard)",
        m_state.targetVerse.book, m_state.targetVerse.chapter, m_state.targetVerse.verse);
    
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(shareText));
        wxTheClipboard->Close();
    }
    
    m_submitBtn->SetLabel("Copied to Clipboard");
}
