#pragma once

#include <wx/wx.h>
#include <functional>

// Hard Mode toggle + seed entry/randomize/readout, matching the web
// version's Settings screen. Seed and hard-mode apply to the next game
// started, same as the web version.
class SettingsScreen : public wxPanel {
public:
    using Callback = std::function<void()>;

    explicit SettingsScreen(wxWindow* parent);

    bool GetHardMode() const;
    wxString GetSeedText() const;
    void SetSeedText(const wxString& seed);
    void SetCurrentSeedDisplay(const wxString& seed);

    void SetOnBack(Callback cb) { m_onBack = std::move(cb); }
    void SetOnRandomizeSeed(Callback cb) { m_onRandomizeSeed = std::move(cb); }

private:
    void SetupUi();

    wxCheckBox* m_hardModeCheck = nullptr;
    wxTextCtrl* m_seedInput = nullptr;
    wxStaticText* m_currentSeedDisplay = nullptr;

    Callback m_onBack;
    Callback m_onRandomizeSeed;
};
