#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>
#include <set>
#include <string>

#include "BibleData.h"
#include "StoryMap.h"
#include "StoryProgress.h"

// Story Mode's map: the Bible lands picture (StoryWorldMap) with a pin on every
// region that has levels, and under it the selected region's levels with their
// area, best stars, Play / Retry / Continue buttons and a per-level Hard Mode
// toggle (unlocked at 4+ stars). Locked bonus levels and a locked bonus area
// are not shown at all. Rebuilt from scratch by ShowMap() whenever progress
// changes or another region is picked.
class StoryMapScreen : public wxScrolledWindow {
public:
    using BackCallback = std::function<void()>;
    using PlayCallback = std::function<void(const std::string& levelKey, bool hardMode)>;

    explicit StoryMapScreen(wxWindow* parent);

    void SetOnBack(BackCallback cb) { m_onBack = std::move(cb); }
    void SetOnPlay(PlayCallback cb) { m_onPlay = std::move(cb); }

    void ShowMap(const StoryMap& map, const StoryProgress& progress, const BibleData& bible, bool allowMature);

private:
    void Rebuild();
    void AddLevelRow(wxSizer* sizer, const StoryLevel& level, const wxString& areaTitle,
                     const StoryProgress& progress, const BibleData& bible, bool allowMature,
                     const std::set<std::string>& nextUp);

    // What the last ShowMap() was given (owned by BibirbleWindow), so picking
    // a region can rebuild the screen.
    const StoryMap* m_map = nullptr;
    const StoryProgress* m_progress = nullptr;
    const BibleData* m_bible = nullptr;
    bool m_allowMature = false;
    std::string m_selectedRegion;  // "" = pick one (where you're up to)
    wxImage m_picture;             // data/story_map.png

    BackCallback m_onBack;
    PlayCallback m_onPlay;
};

// "★★★★☆☆☆": a level's best stars out of its maximum.
wxString StarString(int stars, int maxStars);
