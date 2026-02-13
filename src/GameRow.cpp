#include "GameRow.h"

GameRow::GameRow(wxWindow* parent, const wxArrayString& books)
    : wxPanel(parent) {
    SetupUi(books);
    setDisabled(true);
}
wxTextCtrl* GameRow::CreateDigitInput() {
    wxTextCtrl* edit = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, 
                                      wxSize(40, 40));
    edit->SetMaxLength(1);
    wxFont font(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    edit->SetFont(font);
    return edit;
}
void GameRow::SetupUi(const wxArrayString& books) {
    wxBoxSizer* layout = new wxBoxSizer(wxHORIZONTAL);
    layout->SetMinSize(wxSize(-1, 50));
    
    // Book Dropdown
    m_bookSelect = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, 
                                   wxSize(150, -1), books, wxCB_READONLY);
    m_bookSelect->SetMinSize(wxSize(150, 40));
    m_bookSelect->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& event) {
        if (m_isBookLocked) {
            m_bookSelect->SetStringSelection(m_lockedBookSelection);
            return;
        }
        m_lockedBookSelection = m_bookSelect->GetStringSelection();
        event.Skip();
    });
    layout->Add(m_bookSelect, 2, wxEXPAND | wxALL, 4);
    
    // Chapter inputs
    m_c1 = CreateDigitInput();
    layout->Add(m_c1, 0, wxEXPAND | wxALL, 4);
    
    m_c2 = CreateDigitInput();
    layout->Add(m_c2, 0, wxEXPAND | wxALL, 4);
    
    // Colon separator
    wxStaticText* sep = new wxStaticText(this, wxID_ANY, ":");
    wxFont sepFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    sep->SetFont(sepFont);
    layout->Add(sep, 0, wxALIGN_CENTER | wxALL, 4);
    
    // Verse inputs
    m_v1 = CreateDigitInput();
    layout->Add(m_v1, 0, wxEXPAND | wxALL, 4);
    
    m_v2 = CreateDigitInput();
    layout->Add(m_v2, 0, wxEXPAND | wxALL, 4);
    
    SetSizer(layout);
}



void GameRow::setDisabled(bool disabled) {
    if (!disabled) {
        m_isBookLocked = false;
        m_lockedBookSelection = m_bookSelect->GetStringSelection();
    }

    m_bookSelect->Enable(!disabled);
    m_c1->Enable(!disabled);
    m_c2->Enable(!disabled);
    m_v1->Enable(!disabled);
    m_v2->Enable(!disabled);
    m_c1->SetEditable(!disabled);
    m_c2->SetEditable(!disabled);
    m_v1->SetEditable(!disabled);
    m_v2->SetEditable(!disabled);
}

bool GameRow::isComplete() const {
    if (m_bookSelect->GetStringSelection().IsEmpty()) return false;
    if (m_c1->GetValue().IsEmpty()) return false;
    if (m_c2->GetValue().IsEmpty()) return false;
    if (m_v1->GetValue().IsEmpty()) return false;
    if (m_v2->GetValue().IsEmpty()) return false;
    return true;
}

std::string GameRow::getBook() const {
    return std::string(m_bookSelect->GetStringSelection().mb_str());
}

std::vector<std::string> GameRow::getDigits() const {
    return {
        std::string(m_c1->GetValue().mb_str()),
        std::string(m_c2->GetValue().mb_str()),
        std::string(m_v1->GetValue().mb_str()),
        std::string(m_v2->GetValue().mb_str())
    };
}

void GameRow::setBookColor(const std::string& color) {
    wxColour bgColor;
    wxColour fgColor(*wxBLACK);
    
    if (color == "green") {
        bgColor = *wxGREEN;
        fgColor = *wxWHITE;
    } else if (color == "yellow") {
        bgColor = *wxYELLOW;
        fgColor = *wxBLACK;
    } else {
        bgColor = *wxLIGHT_GREY;
        fgColor = *wxBLACK;
    }
    
    m_bookSelect->SetBackgroundColour(bgColor);
    m_bookSelect->SetForegroundColour(fgColor);
    m_bookSelect->Refresh();
}

void GameRow::setDigitColors(const std::vector<std::string>& colors) {
    std::vector<wxTextCtrl*> edits = {m_c1, m_c2, m_v1, m_v2};
    
    for (size_t i = 0; i < colors.size() && i < edits.size(); ++i) {
        wxColour bgColor;
        wxColour fgColor(*wxBLACK);
        
        if (colors[i] == "green") {
            bgColor = *wxGREEN;
            fgColor = *wxWHITE;
        } else if (colors[i] == "yellow") {
            bgColor = *wxYELLOW;
            fgColor = *wxBLACK;
        } else {
            bgColor = *wxLIGHT_GREY;
            fgColor = *wxBLACK;
        }
        
        edits[i]->SetBackgroundColour(bgColor);
        edits[i]->SetForegroundColour(fgColor);
        edits[i]->Refresh();
    }
}

std::vector<wxTextCtrl*> GameRow::GetDigitCtrls() {
    return { m_c1, m_c2, m_v1, m_v2 };
}

void GameRow::lockSubmitted() {
    m_isBookLocked = true;
    m_lockedBookSelection = m_bookSelect->GetStringSelection();

    // Keep controls visually enabled so custom result colours remain visible.
    m_bookSelect->Enable(true);
    m_c1->Enable(true);
    m_c2->Enable(true);
    m_v1->Enable(true);
    m_v2->Enable(true);
    m_c1->SetEditable(false);
    m_c2->SetEditable(false);
    m_v1->SetEditable(false);
    m_v2->SetEditable(false);
}
