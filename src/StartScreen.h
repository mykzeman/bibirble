#pragma once

#include <wx/wx.h>
#include <functional>

// Mode-select landing screen: Start Daily / Start Random / Settings, an
// inline how-to-play card, and a live countdown to the next UTC daily reset.
class StartScreen : public wxPanel {
public:
    using Callback = std::function<void()>;

    explicit StartScreen(wxWindow* parent);

    void SetOnStartDaily(Callback cb) { m_onStartDaily = std::move(cb); }
    void SetOnStartRandom(Callback cb) { m_onStartRandom = std::move(cb); }
    void SetOnOpenSettings(Callback cb) { m_onOpenSettings = std::move(cb); }

private:
    void SetupUi();
    void UpdateCountdown(wxTimerEvent& event);

    wxStaticText* m_countdownText = nullptr;
    wxTimer m_timer;

    Callback m_onStartDaily;
    Callback m_onStartRandom;
    Callback m_onOpenSettings;
};
