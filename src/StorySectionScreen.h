#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>

#include "IllustrationPanel.h"
#include "StoryData.h"
#include "StoryProgress.h"

// Section detail screen: illustration, description, and a list of chapter
// tiles plus the bonus prophecy chapter tile (locked until the section is
// aced -- every chapter completed with a perfect, no-losses run).
class StorySectionScreen : public wxPanel {
public:
    // chapterIndex >= 0 selects a regular chapter; -1 selects the bonus chapter.
    using ChapterCallback = std::function<void(int chapterIndex)>;
    using VoidCallback = std::function<void()>;

    explicit StorySectionScreen(wxWindow* parent);

    void SetOnChapterSelected(ChapterCallback cb) { m_onChapterSelected = std::move(cb); }
    void SetOnBack(VoidCallback cb) { m_onBack = std::move(cb); }

    void ShowSection(const StorySection& section, const StoryProgress& progress);

private:
    void SetupUi();

    IllustrationPanel* m_illustration = nullptr;
    wxStaticText* m_titleText = nullptr;
    wxStaticText* m_descriptionText = nullptr;
    wxScrolledWindow* m_scrollArea = nullptr;
    wxBoxSizer* m_chapterListSizer = nullptr;

    ChapterCallback m_onChapterSelected;
    VoidCallback m_onBack;
};
