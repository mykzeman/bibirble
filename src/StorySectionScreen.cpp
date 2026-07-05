#include "StorySectionScreen.h"

StorySectionScreen::StorySectionScreen(wxWindow* parent) : wxPanel(parent) {
    SetupUi();
}

void StorySectionScreen::SetupUi() {
    wxColour bgColor(234, 229, 159);
    SetBackgroundColour(bgColor);

    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    m_illustration = new IllustrationPanel(this, "", "");
    layout->Add(m_illustration, 0, wxEXPAND | wxALL, 8);

    m_titleText = new wxStaticText(this, wxID_ANY, "");
    wxFont titleFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);
    layout->Add(m_titleText, 0, wxALL | wxALIGN_CENTER, 8);

    m_descriptionText = new wxStaticText(this, wxID_ANY, "");
    m_descriptionText->Wrap(420);
    layout->Add(m_descriptionText, 0, wxALL | wxEXPAND, 8);

    m_scrollArea = new wxScrolledWindow(this);
    m_scrollArea->SetScrollRate(5, 5);
    m_scrollArea->SetBackgroundColour(bgColor);

    m_chapterListSizer = new wxBoxSizer(wxVERTICAL);
    m_scrollArea->SetSizer(m_chapterListSizer);
    layout->Add(m_scrollArea, 1, wxEXPAND | wxALL, 8);

    wxButton* backBtn = new wxButton(this, wxID_ANY, "Back to Map");
    backBtn->SetBackgroundColour(wxColour(200, 100, 50));
    backBtn->SetForegroundColour(wxColour(235, 230, 157));
    backBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_onBack) m_onBack();
    });
    layout->Add(backBtn, 0, wxEXPAND | wxALL, 8);

    SetSizer(layout);
}

void StorySectionScreen::ShowSection(const StorySection& section, const StoryProgress& progress) {
    m_titleText->SetLabel(wxString::Format("%s (%s)", section.title, section.subtitle));
    m_descriptionText->SetLabel(section.description);
    m_descriptionText->Wrap(420);

    wxString illustrationPath = wxString::Format("section_%02d.png", section.id);
    m_illustration->SetContent(illustrationPath, section.title);

    m_chapterListSizer->Clear(true);

    if (section.chapters.empty()) {
        wxStaticText* comingSoon = new wxStaticText(m_scrollArea, wxID_ANY,
            "This section is still being written. Check back soon!");
        m_chapterListSizer->Add(comingSoon, 0, wxALL | wxALIGN_CENTER, 20);
    } else {
        int chapterCount = (int)section.chapters.size();
        for (int i = 0; i < chapterCount; ++i) {
            const StoryChapter& chapter = section.chapters[i];
            bool completed = progress.IsChapterCompleted(section.id, i);

            wxString label = wxString::Format("Chapter %d: %s%s", i + 1, chapter.title,
                                               completed ? "  [Completed]" : "");
            wxButton* tile = new wxButton(m_scrollArea, wxID_ANY, label, wxDefaultPosition, wxSize(-1, 44));
            tile->SetBackgroundColour(wxColour(200, 100, 50));
            tile->SetForegroundColour(wxColour(235, 230, 157));

            tile->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent&) {
                if (m_onChapterSelected) m_onChapterSelected(i);
            });

            m_chapterListSizer->Add(tile, 0, wxEXPAND | wxALL, 5);
        }

        if (section.hasBonusChapter) {
            bool aced = progress.IsSectionAced(section.id, chapterCount);
            bool bonusDone = progress.IsBonusCompleted(section.id);

            wxString label = wxString::Format("Bonus Prophecy: %s%s", section.bonusChapter.title,
                                               !aced ? "  [Locked -- ace this section]"
                                                     : (bonusDone ? "  [Completed]" : "  [Unlocked!]"));
            wxButton* bonusTile = new wxButton(m_scrollArea, wxID_ANY, label, wxDefaultPosition, wxSize(-1, 44));
            bonusTile->SetBackgroundColour(aced ? wxColour(160, 120, 20) : wxColour(120, 120, 120));
            bonusTile->SetForegroundColour(wxColour(235, 230, 157));
            bonusTile->Enable(aced);

            bonusTile->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
                if (m_onChapterSelected) m_onChapterSelected(-1);
            });

            m_chapterListSizer->Add(bonusTile, 0, wxEXPAND | wxALL, 5);
        }
    }

    m_scrollArea->FitInside();
    m_scrollArea->Layout();
    Layout();
}
