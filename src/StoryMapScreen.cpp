#include "StoryMapScreen.h"
#include "EmbeddedData.h"
#include "StoryWorldMap.h"
#include "Theme.h"
#include "WrappingText.h"

#include <wx/mstream.h>

#include <map>

namespace {
const char* kStar = "\xE2\x98\x85";        // ★
const char* kEmptyStar = "\xE2\x98\x86";   // ☆
const char* kLock = "\xF0\x9F\x94\x92";    // 🔒
const char* kCrystal = "\xF0\x9F\x94\xAE"; // 🔮
const char* kBook = "\xF0\x9F\x93\x96";    // 📖
const char* kPin = "\xF0\x9F\x93\x8D";     // 📍

wxFont BoldFont(int size) {
    return wxFont(size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
}

wxButton* AccentButton(wxWindow* parent, const wxString& label) {
    wxButton* btn = new wxButton(parent, wxID_ANY, label);
    btn->SetBackgroundColour(Theme::Accent());
    btn->SetForegroundColour(Theme::AccentText());
    btn->SetFont(BoldFont(11));
    return btn;
}
}  // namespace

wxString StarString(int stars, int maxStars) {
    wxString out;
    for (int i = 0; i < maxStars; ++i) out += wxString::FromUTF8(i < stars ? kStar : kEmptyStar);
    return out;
}

StoryMapScreen::StoryMapScreen(wxWindow* parent) : wxScrolledWindow(parent) {
    SetScrollRate(0, 10);
    SetBackgroundColour(Theme::Background());
    std::string png;
    if (ReadStoryMapImage(png)) {
        if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG)) wxImage::AddHandler(new wxPNGHandler);
        wxMemoryInputStream stream(png.data(), png.size());
        m_picture.LoadFile(stream, wxBITMAP_TYPE_PNG);
    }
}

void StoryMapScreen::Rebuild() {
    if (m_map && m_progress && m_bible) ShowMap(*m_map, *m_progress, *m_bible, m_allowMature);
}

void StoryMapScreen::ShowMap(const StoryMap& map, const StoryProgress& progress, const BibleData& bible,
                             bool allowMature) {
    m_map = &map;
    m_progress = &progress;
    m_bible = &bible;
    m_allowMature = allowMature;
    Freeze();
    DestroyChildren();
    wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

    // Header: back button, title, total stars.
    wxBoxSizer* header = new wxBoxSizer(wxHORIZONTAL);
    wxButton* back = AccentButton(this, "Main Menu");
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { if (m_onBack) m_onBack(); });
    header->Add(back, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxStaticText* title = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(kBook) + " Story Mode");
    title->SetFont(BoldFont(16));
    header->Add(title, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxStaticText* total = new wxStaticText(this, wxID_ANY,
        wxString::FromUTF8(kStar) + wxString::Format(" %d", progress.TotalStars()));
    total->SetFont(BoldFont(14));
    header->Add(total, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    layout->Add(header, 0, wxEXPAND | wxALL, 5);

    if (!map.IsLoaded()) {
        wxString msg = "Story Mode couldn't load its map:\n";
        for (const auto& e : map.Errors()) msg += "\n- " + wxString::FromUTF8(e);
        WrappingText* err = new WrappingText(this, msg);
        layout->Add(err, 0, wxEXPAND | wxALL, 10);
        SetSizer(layout);
        Thaw();
        return;
    }

    // An attempt in progress: offer to continue it.
    const StoryRun& run = progress.Run();
    if (run.active) {
        const StoryLevel* lvl = map.FindLevel(run.levelKey);
        if (lvl && lvl->available) {
            wxPanel* card = new wxPanel(this);
            card->SetBackgroundColour(*wxWHITE);
            wxBoxSizer* cs = new wxBoxSizer(wxHORIZONTAL);
            WrappingText* t = new WrappingText(card, wxString::Format("In progress: %s (verse %d of %d)",
                wxString::FromUTF8(lvl->title), run.solvedCount + 1, StoryProgress::kVersesPerLevel));
            cs->Add(t, 1, wxALL | wxALIGN_CENTER_VERTICAL, 8);
            wxButton* cont = AccentButton(card, "Continue");
            const std::string key = run.levelKey;
            const bool hard = run.hardMode;
            cont->Bind(wxEVT_BUTTON, [this, key, hard](wxCommandEvent&) { if (m_onPlay) m_onPlay(key, hard); });
            cs->Add(cont, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
            card->SetSizer(cs);
            layout->Add(card, 0, wxEXPAND | wxALL, 8);
        }
    }

    // The next locked level on each path (its predecessor is passed), which
    // shows how many stars it needs.
    std::set<std::string> nextUp;
    for (const auto& path : map.Paths()) {
        for (size_t i = 1; i < path.size(); ++i) {
            if (progress.IsUnlocked(path[i])) continue;
            const LevelProgress* prev = progress.Get(path[i - 1]);
            if (prev && prev->passed) nextUp.insert(path[i]);
            break;
        }
    }

    // The levels that are shown (locked bonus levels and a locked bonus area
    // stay hidden), grouped by the region on the map where they happen.
    struct Shown {
        const StoryLevel* level;
        wxString areaTitle;
    };
    std::map<std::string, std::vector<Shown>> byRegion;
    std::vector<std::string> playOrder;  // levels in map order, to find where the player is up to
    for (const auto& area : map.Areas()) {
        if (area.bonusArea) {
            // Hidden completely until its first level unlocks.
            if (area.levels.empty() || !progress.IsUnlocked(area.levels.front().key)) continue;
        }
        const wxString areaTitle = (area.bonusArea ? wxString::FromUTF8(kCrystal) + " " : wxString()) +
                                   wxString::FromUTF8(area.title);
        for (const auto& lvl : area.levels) {
            byRegion[lvl.region].push_back({&lvl, areaTitle});
            playOrder.push_back(lvl.key);
        }
        for (const auto& lvl : area.bonusLevels) {
            if (progress.IsUnlocked(lvl.key)) byRegion[lvl.region].push_back({&lvl, areaTitle});
        }
    }

    // One pin per region with levels: gold when every level there is passed,
    // orange when something there can be played, grey while it's all locked.
    std::vector<StoryWorldMap::Pin> pins;
    for (const auto& region : map.Regions()) {
        auto it = byRegion.find(region.id);
        if (it == byRegion.end()) continue;
        bool anyOpen = false, allPassed = true;
        for (const auto& shown : it->second) {
            const LevelProgress* lp = progress.Get(shown.level->key);
            const bool unlocked = progress.IsUnlocked(shown.level->key);
            if (unlocked && shown.level->available) anyOpen = true;
            if (!(lp && lp->passed)) allPassed = false;
        }
        StoryWorldMap::Pin pin;
        pin.id = region.id;
        pin.name = wxString::FromUTF8(region.name);
        pin.x = region.x;
        pin.y = region.y;
        pin.levels = static_cast<int>(it->second.size());
        pin.state = allPassed ? StoryWorldMap::PinState::Done
                  : anyOpen ? StoryWorldMap::PinState::Open : StoryWorldMap::PinState::Locked;
        pins.push_back(pin);
    }

    // Which region to show: the one picked, else where the attempt in
    // progress is, else the first unlocked level not yet passed.
    if (!byRegion.count(m_selectedRegion)) m_selectedRegion.clear();
    if (m_selectedRegion.empty() && run.active) {
        if (const StoryLevel* lvl = map.FindLevel(run.levelKey)) m_selectedRegion = lvl->region;
    }
    if (m_selectedRegion.empty()) {
        for (const auto& key : playOrder) {
            const StoryLevel* lvl = map.FindLevel(key);
            const LevelProgress* lp = progress.Get(key);
            if (lvl && lvl->available && progress.IsUnlocked(key) && !(lp && lp->passed)) {
                m_selectedRegion = lvl->region;
                break;
            }
        }
    }
    if (m_selectedRegion.empty() && !pins.empty()) m_selectedRegion = pins.front().id;

    StoryWorldMap* worldMap = new StoryWorldMap(this, m_picture);
    worldMap->SetPins(pins, m_selectedRegion);
    worldMap->SetOnSelect([this](const std::string& id) {
        m_selectedRegion = id;
        // Rebuilding destroys the map panel, so not from inside its handler.
        CallAfter([this]() { Rebuild(); });
    });
    layout->Add(worldMap, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

    WrappingText* hint = new WrappingText(this, "Pick a place on the map to see its levels. Orange: levels to play. "
                                                "Gold: all passed. Grey: still locked.");
    hint->SetForegroundColour(wxColour(110, 100, 80));
    layout->Add(hint, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    if (const StoryRegion* region = map.FindRegion(m_selectedRegion)) {
        wxStaticText* regionTitle = new wxStaticText(this, wxID_ANY,
            wxString::FromUTF8(kPin) + " " + wxString::FromUTF8(region->name));
        regionTitle->SetFont(BoldFont(14));
        regionTitle->SetForegroundColour(Theme::Accent());
        layout->Add(regionTitle, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        for (const auto& shown : byRegion[m_selectedRegion]) {
            AddLevelRow(layout, *shown.level, shown.areaTitle, progress, bible, allowMature, nextUp);
        }
    }
    layout->AddSpacer(10);

    SetSizer(layout);
    FitInside();
    Thaw();
    Layout();
}

void StoryMapScreen::AddLevelRow(wxSizer* sizer, const StoryLevel& level, const wxString& areaTitle,
                                 const StoryProgress& progress, const BibleData& bible, bool allowMature,
                                 const std::set<std::string>& nextUp) {
    wxPanel* row = new wxPanel(this);
    const bool unlocked = progress.IsUnlocked(level.key);
    row->SetBackgroundColour(unlocked ? *wxWHITE : wxColour(225, 225, 225));
    wxBoxSizer* rs = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* info = new wxBoxSizer(wxVERTICAL);
    // Which part of the story the level belongs to ("Promises").
    wxStaticText* area = new wxStaticText(row, wxID_ANY, areaTitle);
    area->SetForegroundColour(wxColour(150, 120, 80));
    info->Add(area, 0, wxBOTTOM, 1);

    const wxString name = (level.isBonus ? wxString::FromUTF8(kCrystal) + " " : wxString()) +
                          wxString::FromUTF8(level.title);
    if (!unlocked) {
        wxStaticText* t = new wxStaticText(row, wxID_ANY, wxString::FromUTF8(kLock) + " " + name);
        t->SetForegroundColour(wxColour(120, 120, 120));
        info->Add(t, 0, wxBOTTOM, 2);
        if (nextUp.count(level.key)) {
            wxStaticText* need = new wxStaticText(row, wxID_ANY, wxString::Format("Needs %d %s (you have %d)",
                progress.StarsNeeded(level), wxString::FromUTF8(kStar), progress.TotalStars()));
            need->SetForegroundColour(wxColour(120, 120, 120));
            info->Add(need, 0);
        }
        rs->Add(info, 1, wxALL, 8);
        row->SetSizer(rs);
        sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
        return;
    }

    const LevelProgress* lp = progress.Get(level.key);
    const int stars = lp ? lp->stars.Total() : 0;
    if (!level.available) {
        // This Bible version doesn't have the level's verses.
        row->SetBackgroundColour(wxColour(235, 235, 235));
        wxStaticText* t = new wxStaticText(row, wxID_ANY, name);
        t->SetFont(BoldFont(11));
        info->Add(t, 0, wxBOTTOM, 2);
        info->Add(new wxStaticText(row, wxID_ANY, StarString(stars, level.MaxStars())), 0, wxBOTTOM, 2);
        WrappingText* note = new WrappingText(row, wxString::FromUTF8(
            "Not in the " + std::string(bible.version().name) + ". Pick another Bible in Settings to play it."));
        note->SetForegroundColour(wxColour(120, 120, 120));
        info->Add(note, 0, wxEXPAND);
        rs->Add(info, 1, wxALL, 8);
        row->SetSizer(rs);
        sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
        return;
    }
    wxStaticText* t = new wxStaticText(row, wxID_ANY, name);
    t->SetFont(BoldFont(11));
    info->Add(t, 0, wxBOTTOM, 2);
    wxStaticText* starText = new wxStaticText(row, wxID_ANY, StarString(stars, level.MaxStars()));
    starText->SetForegroundColour(wxColour(200, 150, 0));
    starText->SetFont(BoldFont(13));
    info->Add(starText, 0, wxBOTTOM, 2);
    const int poolSize = static_cast<int>(StoryProgress::PlayablePool(level, bible, allowMature).size());
    info->Add(new wxStaticText(row, wxID_ANY, wxString::Format("Pool %d / %d",
        progress.PerfectProgress(level, bible, allowMature), poolSize)), 0);
    rs->Add(info, 1, wxALL, 8);

    wxBoxSizer* actions = new wxBoxSizer(wxVERTICAL);
    const bool resumable = progress.Run().active && progress.Run().levelKey == level.key;
    const bool hardUnlocked = progress.HardModeUnlocked(level.key) && level.allowHard;
    wxCheckBox* hard = nullptr;
    if (!resumable) {
        hard = new wxCheckBox(row, wxID_ANY, hardUnlocked ? wxString("Hard Mode")
            : "Hard Mode (4" + wxString::FromUTF8(kStar) + ")");
        hard->Enable(hardUnlocked);
        if (!hardUnlocked) hard->SetToolTip("Earn 4 or more stars on this level to unlock Hard Mode.");
    }
    const wxString label = resumable ? "Continue" : (lp && lp->passed ? "Retry" : "Play");
    wxButton* play = AccentButton(row, label);
    const std::string key = level.key;
    const bool runHard = resumable && progress.Run().hardMode;
    play->Bind(wxEVT_BUTTON, [this, key, hard, runHard](wxCommandEvent&) {
        if (m_onPlay) m_onPlay(key, hard ? hard->GetValue() : runHard);
    });
    actions->Add(play, 0, wxEXPAND | wxBOTTOM, 4);
    if (hard) actions->Add(hard, 0);
    rs->Add(actions, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);

    row->SetSizer(rs);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
}
