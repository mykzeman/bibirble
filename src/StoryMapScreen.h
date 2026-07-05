#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>

#include "IllustrationPanel.h"
#include "StoryData.h"
#include "StoryProgress.h"

// World-map / section-select screen: a scrollable list of the 10 story
// section tiles (locked/unlocked/aced state), with a banner illustration at
// the top. mapX/mapY on StorySection are kept in the data for a future pass
// that lays tiles out over a real map image; for now each tile is a simple
// clickable list entry so the flow is robust without art assets.
class StoryMapScreen : public wxPanel {
public:
    using SectionCallback = std::function<void(int sectionId)>;
    using VoidCallback = std::function<void()>;

    explicit StoryMapScreen(wxWindow* parent);

    void SetOnSectionSelected(SectionCallback cb) { m_onSectionSelected = std::move(cb); }
    void SetOnBack(VoidCallback cb) { m_onBack = std::move(cb); }

    // Rebuilds the section tiles from current data + progress. Call before showing.
    void RefreshSections(const StoryData& storyData, const StoryProgress& progress);

private:
    void SetupUi();

    IllustrationPanel* m_banner = nullptr;
    wxScrolledWindow* m_scrollArea = nullptr;
    wxBoxSizer* m_listSizer = nullptr;

    SectionCallback m_onSectionSelected;
    VoidCallback m_onBack;
};
