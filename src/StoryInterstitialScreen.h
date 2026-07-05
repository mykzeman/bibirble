#pragma once

#include <wx/wx.h>
#include <functional>

#include "IllustrationPanel.h"

// Reusable "story beat" screen: an illustration, a title, a message, and a
// primary action button. Used both for chapter intros ("Begin") and chapter
// completion summaries ("Continue") so the two don't need separate classes.
class StoryInterstitialScreen : public wxPanel {
public:
    using VoidCallback = std::function<void()>;

    explicit StoryInterstitialScreen(wxWindow* parent);

    // Called each time this is shown, since the meaning of "continue" and
    // "back" changes depending on which story beat is being displayed.
    void SetOnContinue(VoidCallback cb) { m_onContinue = std::move(cb); }
    void SetOnBack(VoidCallback cb) { m_onBack = std::move(cb); }

    void Present(const wxString& illustrationAssetPath, const wxString& title, const wxString& message,
                 const wxString& continueLabel, bool showBackLink);

private:
    void SetupUi();

    IllustrationPanel* m_illustration = nullptr;
    wxStaticText* m_titleText = nullptr;
    wxStaticText* m_messageText = nullptr;
    wxButton* m_continueBtn = nullptr;
    wxButton* m_backBtn = nullptr;

    VoidCallback m_onContinue;
    VoidCallback m_onBack;
};
