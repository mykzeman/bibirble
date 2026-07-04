#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(wxWindow* parent) : wxPanel(parent) {
    SetupUi();
}

void SettingsScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* header = new wxBoxSizer(wxHORIZONTAL);
    wxButton* backBtn = new wxButton(this, wxID_ANY, "Back");
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
    randomizeBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onRandomizeSeed) m_onRandomizeSeed();
    });
    cardLayout->Add(randomizeBtn, 0, wxALIGN_CENTER | wxALL, 8);

    wxStaticText* note = new wxStaticText(card, wxID_ANY,
        "Seed settings and hard mode apply to the next game you start. "
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
