#pragma once

#include <wx/wx.h>
#include <functional>

// Hard Mode, Useful-verses-only and Book hints toggles + seed entry/randomize/readout, matching the web
// version's Settings screen. Seed and hard-mode apply to the next game
// started, same as the web version.
class SettingsScreen : public wxPanel {
public:
    using Callback = std::function<void()>;

    explicit SettingsScreen(wxWindow* parent);

    bool GetHardMode() const;
    bool GetUsefulOnly() const;
    // Book hints are an assist, so they're off whenever hard mode is on.
    bool GetBookHints() const;
    wxString GetSeedText() const;
    void SetSeedText(const wxString& seed);
    void SetCurrentSeedDisplay(const wxString& seed);

    void SetOnBack(Callback cb) { m_onBack = std::move(cb); }
    void SetOnRandomizeSeed(Callback cb) { m_onRandomizeSeed = std::move(cb); }

private:
    void SetupUi();

    wxCheckBox* m_hardModeCheck = nullptr;
    wxCheckBox* m_usefulOnlyCheck = nullptr;
    wxCheckBox* m_bookHintsCheck = nullptr;
    wxTextCtrl* m_seedInput = nullptr;
    wxStaticText* m_currentSeedDisplay = nullptr;

    Callback m_onBack;
    Callback m_onRandomizeSeed;
};
