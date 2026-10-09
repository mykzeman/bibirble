#include "SettingsScreen.h"

#include <wx/datectrl.h>
#include <wx/datetime.h>

#include "AgeCheck.h"

namespace {
void StyleButton(wxButton* btn) {
    btn->SetBackgroundColour(wxColour(200, 100, 50));
    btn->SetForegroundColour(wxColour(235, 230, 157));
    wxFont btnFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    btn->SetFont(btnFont);
}
}  // namespace

SettingsScreen::SettingsScreen(wxWindow* parent) : wxPanel(parent) {
    SetupUi();
}

void SettingsScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* header = new wxBoxSizer(wxHORIZONTAL);
    wxButton* backBtn = new wxButton(this, wxID_ANY, "Back");
    StyleButton(backBtn);
    backBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onBack) m_onBack();
    });
    header->Add(backBtn, 0, wxALL, 5);

    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Settings");
    wxFont titleFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    header->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    layout->Add(header, 0, wxEXPAND);

    wxPanel* card = new wxPanel(this);
    card->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* cardLayout = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* hardModeRow = new wxBoxSizer(wxHORIZONTAL);
    hardModeRow->Add(new wxStaticText(card, wxID_ANY, "Hard mode"), 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_hardModeCheck = new wxCheckBox(card, wxID_ANY, "");
    hardModeRow->Add(m_hardModeCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(hardModeRow, 0, wxEXPAND);

    wxBoxSizer* usefulRow = new wxBoxSizer(wxHORIZONTAL);
    usefulRow->Add(new wxStaticText(card, wxID_ANY, "Useful verses only"), 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_usefulOnlyCheck = new wxCheckBox(card, wxID_ANY, "");
    usefulRow->Add(m_usefulOnlyCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(usefulRow, 0, wxEXPAND);

    wxBoxSizer* hintsRow = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* hintsLabel = new wxStaticText(card, wxID_ANY,
        "Book hints (narrows the book list using your clues; not in hard mode)");
    hintsLabel->Wrap(300);
    hintsRow->Add(hintsLabel, 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_bookHintsCheck = new wxCheckBox(card, wxID_ANY, "");
    hintsRow->Add(m_bookHintsCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(hintsRow, 0, wxEXPAND);

    wxBoxSizer* r18Row = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* r18Label = new wxStaticText(card, wxID_ANY,
        "R18 mode (lets Daily and Random games pick verses with mature content; "
        "18+ only; Useful verses only always stays family friendly)");
    r18Label->Wrap(300);
    r18Row->Add(r18Label, 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_r18Check = new wxCheckBox(card, wxID_ANY, "");
    r18Row->Add(m_r18Check, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(r18Row, 0, wxEXPAND);

    m_r18Check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        if (m_r18Check->GetValue() && !m_r18Verified && !VerifyAge()) {
            m_r18Check->SetValue(false);
        }
        event.Skip();
    });

    m_hardModeCheck->Bind(wxEVT_CHECKBOX, [this, hintsLabel](wxCommandEvent& event) {
        bool hard = m_hardModeCheck->GetValue();
        m_bookHintsCheck->Enable(!hard);
        hintsLabel->Enable(!hard);
        event.Skip();
    });

    wxBoxSizer* seedRow = new wxBoxSizer(wxHORIZONTAL);
    seedRow->Add(new wxStaticText(card, wxID_ANY, "Random seed"), 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_seedInput = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition, wxSize(150, -1));
    m_seedInput->SetHint("enter seed");
    seedRow->Add(m_seedInput, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(seedRow, 0, wxEXPAND);

    wxBoxSizer* currentSeedRow = new wxBoxSizer(wxHORIZONTAL);
    currentSeedRow->Add(new wxStaticText(card, wxID_ANY, "Current seed"), 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_currentSeedDisplay = new wxStaticText(card, wxID_ANY, "none");
    currentSeedRow->Add(m_currentSeedDisplay, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    cardLayout->Add(currentSeedRow, 0, wxEXPAND);

    wxButton* randomizeBtn = new wxButton(card, wxID_ANY, "Randomize Seed");
    StyleButton(randomizeBtn);
    randomizeBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onRandomizeSeed) m_onRandomizeSeed();
    });
    cardLayout->Add(randomizeBtn, 0, wxALIGN_CENTER | wxALL, 8);

    wxStaticText* note = new wxStaticText(card, wxID_ANY,
        "Seed settings, hard mode, useful verses only, book hints, and R18 mode apply to the next game you start. "
        "You can return here anytime.");
    note->Wrap(400);
    cardLayout->Add(note, 0, wxALL, 8);

    card->SetSizer(cardLayout);
    layout->Add(card, 0, wxEXPAND | wxALL, 10);

    SetSizer(layout);
}

bool SettingsScreen::GetHardMode() const {
    return m_hardModeCheck && m_hardModeCheck->GetValue();
}

bool SettingsScreen::GetUsefulOnly() const {
    return m_usefulOnlyCheck && m_usefulOnlyCheck->GetValue();
}

bool SettingsScreen::GetBookHints() const {
    return m_bookHintsCheck && m_bookHintsCheck->GetValue() && !GetHardMode();
}

bool SettingsScreen::GetR18Mode() const {
    return m_r18Check && m_r18Check->GetValue() && m_r18Verified;
}

bool SettingsScreen::VerifyAge() {
    wxDialog dlg(this, wxID_ANY, "R18 mode");
    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);
    layout->Add(new wxStaticText(&dlg, wxID_ANY, "Pick your date of birth:"), 0, wxALL, 10);

    wxDateTime today = wxDateTime::Today();
    wxDatePickerCtrl* picker = new wxDatePickerCtrl(&dlg, wxID_ANY, today, wxDefaultPosition,
                                                    wxDefaultSize, wxDP_DROPDOWN | wxDP_SHOWCENTURY);
    picker->SetRange(wxDateTime(1, wxDateTime::Jan, 1900), today);
    layout->Add(picker, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    layout->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    dlg.SetSizerAndFit(layout);
    dlg.CentreOnParent();

    if (dlg.ShowModal() != wxID_OK) return false;

    wxDateTime birth = picker->GetValue();
    int age = -1;
    if (birth.IsValid() && birth <= today) {
        age = AgeOnDate(birth.GetYear(), birth.GetMonth() + 1, birth.GetDay(),
                        today.GetYear(), today.GetMonth() + 1, today.GetDay());
    }

    if (age < 0) {
        wxMessageBox("Please pick a valid date of birth.", "R18 mode",
                     wxOK | wxICON_WARNING, this);
        return false;
    }
    if (age < kR18MinimumAge) {
        wxMessageBox("Sorry, R18 mode is only for players 18 or older.", "R18 mode",
                     wxOK | wxICON_INFORMATION, this);
        return false;
    }
    m_r18Verified = true;
    return true;
}

wxString SettingsScreen::GetSeedText() const {
    return m_seedInput ? m_seedInput->GetValue().Trim(true).Trim(false) : wxString();
}

void SettingsScreen::SetSeedText(const wxString& seed) {
    if (m_seedInput) m_seedInput->SetValue(seed);
}

void SettingsScreen::SetCurrentSeedDisplay(const wxString& seed) {
    if (!m_currentSeedDisplay) return;
    m_currentSeedDisplay->SetLabel(seed.IsEmpty() ? "none" : seed);
    // The label can grow (e.g. "none" -> a 9-digit seed); reflow so the sizer
    // gives it enough room instead of clipping the new text.
    m_currentSeedDisplay->InvalidateBestSize();
    Layout();
}
