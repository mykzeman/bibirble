#pragma once
#include <wx/wx.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "BibleData.h"
#if defined(__has_include)
#  if __has_include(<wx/callafter.h>)
#    include <wx/callafter.h>
#  else
#    include "wx_callafter_compat.h"
#  endif
#else
#  include <wx/callafter.h>
#endif

// A loader that reads a local JSON array of objects and extracts the "text" fields using nlohmann::json.
// It runs on a background thread, animates the gauge via wxCallAfter, and stores the selected text.
class LoadingDialog : public wxDialog
{
public:
    // path is optional -- defaults to a local file named `bible_sections.json`
    explicit LoadingDialog(wxWindow* parent, const wxString& path = wxEmptyString)
        : wxDialog(parent, wxID_ANY, "Loading", wxDefaultPosition, wxSize(320, 110)),
          m_gauge(new wxGauge(this, wxID_ANY, 100)),
          m_path(path)
    {
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        s->Add(new wxStaticText(this, wxID_ANY, "Loading..."), 0, wxALL | wxALIGN_CENTER, 10);
        s->Add(m_gauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        SetSizerAndFit(s);

        // Launch background worker after the modal loop starts so the gauge is visible.
        wxCallAfter([this]() {
            std::thread([this]() { LoadInBackground(); }).detach();
        });
    }

    // After the dialog finishes (ShowModal returns), call this to obtain the loaded text.
    const std::string& GetLoadedText() const { return m_loadedText; }
    bool FoundFile() const { return m_foundFile; }

private:
    void LoadInBackground()
    {
        const auto startedAt = std::chrono::steady_clock::now();
        constexpr auto kMinLoadVisible = std::chrono::milliseconds(1500);

        const std::string preferredPath = std::string(m_path.mb_str(wxConvUTF8));
        const std::string resolvedPath = BibleData::ResolveDataFilePath(preferredPath);

        std::string fileContent;
        if (!resolvedPath.empty()) {
            std::ifstream ifs(resolvedPath, std::ios::binary);
            if (ifs) {
                std::ostringstream ss;
                ss << ifs.rdbuf();
                fileContent = ss.str();
                m_foundFile = true;
            }
        }

        // Parse using nlohmann::json
        std::vector<std::string> texts;
        if (!fileContent.empty()) {
            try {
                auto j = nlohmann::json::parse(fileContent);
                if (j.is_array()) {
                    for (const auto& item : j) {
                        if (item.contains("text") && item["text"].is_string()) {
                            texts.push_back(item["text"].get<std::string>());
                        }
                    }
                }
            } catch (const std::exception&) {
                // Keep texts empty. Presence/readability is tracked by m_foundFile.
            }
        }

        // Animate progress with a guaranteed minimum visible duration.
        for (int v = 0; v <= 90; v += 5) {
            wxCallAfter([this, v]() { m_gauge->SetValue(v); });
            std::this_thread::sleep_for(std::chrono::milliseconds(55));
        }

        std::mt19937 rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        if (!texts.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, texts.size() - 1);
            std::size_t idx = dist(rng);
            m_loadedText = texts[idx];
        } else {
            m_loadedText = "";
        }

        for (int v = 90; v <= 100; v += 2) {
            wxCallAfter([this, v]() { m_gauge->SetValue(v); });
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }

        const auto elapsed = std::chrono::steady_clock::now() - startedAt;
        if (elapsed < kMinLoadVisible) {
            std::this_thread::sleep_for(kMinLoadVisible - elapsed);
        }

        wxCallAfter([this]() { EndModal(wxID_OK); });
    }

    wxGauge* m_gauge;
    std::string m_loadedText;
    wxString m_path;
    bool m_foundFile = false;
};
