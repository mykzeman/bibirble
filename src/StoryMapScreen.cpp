#include "StoryMapScreen.h"

StoryMapScreen::StoryMapScreen(wxWindow* parent) : wxPanel(parent) {
    SetupUi();
}

void StoryMapScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    wxStaticText* headline = new wxStaticText(this, wxID_ANY, "The Story of Scripture");
    wxFont headlineFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    headline->SetFont(headlineFont);
    layout->Add(headline, 0, wxALL | wxALIGN_CENTER, 10);

    m_banner = new IllustrationPanel(this, "map.png", "The Story of Scripture");
    layout->Add(m_banner, 0, wxEXPAND | wxALL, 8);

    m_scrollArea = new wxScrolledWindow(this);
    m_scrollArea->SetScrollRate(5, 5);
    m_scrollArea->SetBackgroundColour(bgColor);

    m_listSizer = new wxBoxSizer(wxVERTICAL);
    m_scrollArea->SetSizer(m_listSizer);
    layout->Add(m_scrollArea, 1, wxEXPAND | wxALL, 8);

    wxButton* backBtn = new wxButton(this, wxID_ANY, "Back to Main Menu");
    backBtn->SetBackgroundColour(wxColour(200, 100, 50));
    backBtn->SetForegroundColour(wxColour(235, 230, 157));
    backBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onBack) m_onBack();
    });
    layout->Add(backBtn, 0, wxEXPAND | wxALL, 8);

    SetSizer(layout);
}

void StoryMapScreen::RefreshSections(const StoryData& storyData, const StoryProgress& progress) {
    m_listSizer->Clear(true);

    for (const auto& section : storyData.getSections()) {
        int previousChapterCount = 0;
        if (const StorySection* prev = storyData.findSection(section.id - 1)) {
            previousChapterCount = (int)prev->chapters.size();
        }
        bool unlocked = progress.IsSectionUnlocked(section.id, previousChapterCount);
        bool hasContent = !section.chapters.empty();
        bool aced = hasContent && progress.IsSectionAced(section.id, (int)section.chapters.size());
        bool completed = hasContent && progress.AreAllChaptersCompleted(section.id, (int)section.chapters.size());

        wxString status;
        if (!unlocked) {
            status = "[Locked]";
        } else if (!hasContent) {
            status = "[Coming Soon]";
        } else if (aced) {
            status = "[Aced!]";
        } else if (completed) {
            status = "[Completed]";
        } else {
            status = "[Available]";
        }

        wxString label = wxString::Format("%d. %s -- %s\n%s", section.id, section.title, section.subtitle, status);

        wxButton* tile = new wxButton(m_scrollArea, wxID_ANY, label, wxDefaultPosition, wxSize(-1, 60));
        tile->SetBackgroundColour(wxColour(200, 100, 50));
        tile->SetForegroundColour(wxColour(235, 230, 157));
        tile->Enable(unlocked && hasContent);

        int sectionId = section.id;
        tile->Bind(wxEVT_BUTTON, [this, sectionId](wxCommandEvent&) {
            if (m_onSectionSelected) m_onSectionSelected(sectionId);
        });

        m_listSizer->Add(tile, 0, wxEXPAND | wxALL, 5);
    }

    m_scrollArea->FitInside();
    m_scrollArea->Layout();
    Layout();
}
