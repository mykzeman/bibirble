#include "StoryInterstitialScreen.h"

StoryInterstitialScreen::StoryInterstitialScreen(wxWindow* parent) : wxPanel(parent) {
    SetupUi();
}

void StoryInterstitialScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    m_illustration = new IllustrationPanel(this, "", "");
    layout->Add(m_illustration, 0, wxEXPAND | wxALL, 8);

    m_titleText = new wxStaticText(this, wxID_ANY, "");
    wxFont titleFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);
    layout->Add(m_titleText, 0, wxALL | wxALIGN_CENTER, 8);

    m_messageText = new wxStaticText(this, wxID_ANY, "");
    m_messageText->Wrap(420);
    layout->Add(m_messageText, 1, wxALL | wxEXPAND, 8);

    m_continueBtn = new wxButton(this, wxID_ANY, "Continue", wxDefaultPosition, wxSize(-1, 50));
    m_continueBtn->SetBackgroundColour(wxColour(200, 100, 50));
    m_continueBtn->SetForegroundColour(wxColour(235, 230, 157));
    wxFont btnFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_continueBtn->SetFont(btnFont);
    m_continueBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onContinue) m_onContinue();
    });
    layout->Add(m_continueBtn, 0, wxEXPAND | wxALL, 8);

    m_backBtn = new wxButton(this, wxID_ANY, "Back");
    m_backBtn->SetBackgroundColour(wxColour(200, 100, 50));
    m_backBtn->SetForegroundColour(wxColour(235, 230, 157));
    m_backBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onBack) m_onBack();
    });
    layout->Add(m_backBtn, 0, wxEXPAND | wxALL, 8);

    SetSizer(layout);
}

void StoryInterstitialScreen::Present(const wxString& illustrationAssetPath, const wxString& title,
                                       const wxString& message, const wxString& continueLabel,
                                       bool showBackLink) {
    m_illustration->SetContent(illustrationAssetPath, title);
    m_titleText->SetLabel(title);
    m_messageText->SetLabel(message);
    m_messageText->Wrap(420);
    m_continueBtn->SetLabel(continueLabel);

    GetSizer()->Show(m_backBtn, showBackLink);
    Layout();
}
