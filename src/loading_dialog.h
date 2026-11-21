#pragma once
#include <wx/wx.h>
#include <thread>
#include <chrono>
#include <string>
#include "BibleData.h"

// Loads a Bible version (BibleData::loadVersion) on a background thread while showing a gauge, then hands
// it to the caller via TakeData() so the main window doesn't parse the verse file a second time. The gauge
// is animated via wxEvtHandler::CallAfter().
//
// This dialog is shown via ShowModal() from inside wxApp::OnInit(), before the app's main event loop
// has started. That matters: the free-function wxCallAfter(fn) (and this project's old
// wx_callafter_compat.h shim) queue the call onto wxTheApp, which is only drained by the *main* event
// loop's idle processing -- so it would never run while blocked inside a nested ShowModal() loop that
// starts before that main loop does, and EndModal() would never fire. Calling CallAfter() as a member
// function on `this` (the dialog itself) queues it on the dialog's own pending-event list instead,
// which the nested modal loop does drain, so it works correctly here.
class LoadingDialog : public wxDialog
{
public:
    // Loads the version with this id (BibleVersions.h), falling back to the World English Bible if it
    // can't be read -- the same fallback BibirbleWindow::LoadBibleVersion() uses.
    LoadingDialog(wxWindow* parent, const std::string& versionId)
        : wxDialog(parent, wxID_ANY, "Loading", wxDefaultPosition, wxSize(320, 110)),
          m_gauge(new wxGauge(this, wxID_ANY, 100)),
          m_versionId(versionId)
    {
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        s->Add(new wxStaticText(this, wxID_ANY, "Loading..."), 0, wxALL | wxALIGN_CENTER, 10);
        s->Add(m_gauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        SetSizerAndFit(s);

        // Launch background worker after the modal loop starts so the gauge is visible.
        this->CallAfter([this]() {
            std::thread([this]() { LoadInBackground(); }).detach();
        });
    }

    // After the dialog finishes (ShowModal returns), take ownership of the loaded data.
    BibleData TakeData() { return std::move(m_data); }
    bool FoundFile() const { return m_foundFile; }

private:
    void LoadInBackground()
    {
        const auto startedAt = std::chrono::steady_clock::now();
        constexpr auto kMinLoadVisible = std::chrono::milliseconds(1500);

        const BibleVersion& wanted = FindBibleVersion(m_versionId);
        if (!m_data.loadVersion(wanted) && &wanted != &BibleVersions().front()) {
            m_data.loadVersion(BibleVersions().front());
        }
        m_foundFile = m_data.isLoaded();

        // Animate progress with a guaranteed minimum visible duration.
        for (int v = 0; v <= 100; v += 5) {
            this->CallAfter([this, v]() { m_gauge->SetValue(v); });
            std::this_thread::sleep_for(std::chrono::milliseconds(55));
        }

        const auto elapsed = std::chrono::steady_clock::now() - startedAt;
        if (elapsed < kMinLoadVisible) {
            std::this_thread::sleep_for(kMinLoadVisible - elapsed);
        }

        this->CallAfter([this]() { EndModal(wxID_OK); });
    }

    wxGauge* m_gauge;
    BibleData m_data;
    std::string m_versionId;
    bool m_foundFile = false;
};
