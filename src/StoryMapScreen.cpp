#include "StoryMapScreen.h"
#include "Theme.h"

namespace {
const char* kStar = "\xE2\x98\x85";        // ★
const char* kEmptyStar = "\xE2\x98\x86";   // ☆
const char* kLock = "\xF0\x9F\x94\x92";    // 🔒
const char* kCrystal = "\xF0\x9F\x94\xAE"; // 🔮
const char* kBook = "\xF0\x9F\x93\x96";    // 📖

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
}

void StoryMapScreen::ShowMap(const StoryMap& map, const StoryProgress& progress, const BibleData& bible,
                             bool allowMature) {
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
        wxStaticText* err = new wxStaticText(this, wxID_ANY, msg);
        err->Wrap(420);
        layout->Add(err, 0, wxALL, 10);
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
            wxStaticText* t = new wxStaticText(card, wxID_ANY, wxString::Format("In progress: %s (verse %d of %d)",
                wxString::FromUTF8(lvl->title), run.solvedCount + 1, StoryProgress::kVersesPerLevel));
            t->Wrap(300);
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

    for (const auto& area : map.Areas()) {
        if (area.bonusArea) {
            // Hidden completely until its first level unlocks.
            if (area.levels.empty() || !progress.IsUnlocked(area.levels.front().key)) continue;
        }
        wxStaticText* areaTitle = new wxStaticText(this, wxID_ANY,
            (area.bonusArea ? wxString::FromUTF8(kCrystal) + " " : wxString()) + wxString::FromUTF8(area.title));
        areaTitle->SetFont(BoldFont(14));
        areaTitle->SetForegroundColour(Theme::Accent());
        layout->Add(areaTitle, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        for (const auto& lvl : area.levels) AddLevelRow(layout, lvl, progress, bible, allowMature, nextUp);
        for (const auto& lvl : area.bonusLevels) {
            if (progress.IsUnlocked(lvl.key)) AddLevelRow(layout, lvl, progress, bible, allowMature, nextUp);
        }
    }

    SetSizer(layout);
    FitInside();
    Thaw();
    Layout();
}

void StoryMapScreen::AddLevelRow(wxSizer* sizer, const StoryLevel& level, const StoryProgress& progress,
                                 const BibleData& bible, bool allowMature, const std::set<std::string>& nextUp) {
    wxPanel* row = new wxPanel(this);
    const bool unlocked = progress.IsUnlocked(level.key);
    row->SetBackgroundColour(unlocked ? *wxWHITE : wxColour(225, 225, 225));
    wxBoxSizer* rs = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* info = new wxBoxSizer(wxVERTICAL);

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
        wxStaticText* note = new wxStaticText(row, wxID_ANY, wxString::FromUTF8(
            "Not in the " + std::string(bible.version().name) + ". Pick another Bible in Settings to play it."));
        note->SetForegroundColour(wxColour(120, 120, 120));
        note->Wrap(320);
        info->Add(note, 0);
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
