#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>
#include <set>
#include <string>

#include "BibleData.h"
#include "StoryMap.h"
#include "StoryProgress.h"

// Story Mode's map: every area in order with its levels, best stars, Play /
// Retry / Continue buttons and a per-level Hard Mode toggle (unlocked at 4+
// stars). Locked bonus levels and a locked bonus area are not shown at all.
// Rebuilt from scratch by ShowMap() whenever progress changes.
class StoryMapScreen : public wxScrolledWindow {
public:
    using BackCallback = std::function<void()>;
    using PlayCallback = std::function<void(const std::string& levelKey, bool hardMode)>;

    explicit StoryMapScreen(wxWindow* parent);

    void SetOnBack(BackCallback cb) { m_onBack = std::move(cb); }
    void SetOnPlay(PlayCallback cb) { m_onPlay = std::move(cb); }

    void ShowMap(const StoryMap& map, const StoryProgress& progress, const BibleData& bible, bool allowMature);

private:
    void AddLevelRow(wxSizer* sizer, const StoryLevel& level, const StoryProgress& progress,
                     const BibleData& bible, bool allowMature, const std::set<std::string>& nextUp);

    BackCallback m_onBack;
    PlayCallback m_onPlay;
};

// "★★★★☆☆☆": a level's best stars out of its maximum.
wxString StarString(int stars, int maxStars);
