#pragma once
#include <wx/wx.h>
#include <wx/wrapsizer.h>
#include <vector>

// A simple wireframe window for the app (header-only for easy inclusion)
class BibirbleFrame : public wxFrame
{
public:
    BibirbleFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, "Bibirble", wxDefaultPosition, wxSize(600, 800))
    {
        wxColour bgColour(235, 230, 157);
        wxPanel* mainPanel = new wxPanel(this);
        mainPanel->SetBackgroundColour(bgColour);

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* header = new wxStaticText(mainPanel, wxID_ANY, "Bibirble");
        wxFont headerFont = header->GetFont();
        headerFont.SetPointSize(26);
        headerFont.SetWeight(wxFONTWEIGHT_BOLD);
        header->SetFont(headerFont);
        mainSizer->Add(header, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 20);

        wxPanel* revealPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 80));
        revealPanel->SetBackgroundColour(*wxWHITE);
        mainSizer->Add(revealPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 50);

        for (int i = 0; i < 7; ++i) {
            bool isEnabled = (i == 0);
            wxBoxSizer* rowSizer = CreateRow(mainPanel, isEnabled);
            mainSizer->Add(rowSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);
        }

        mainSizer->AddStretchSpacer(1);

        wxPanel* keyboardPanel = new wxPanel(mainPanel);
        keyboardPanel->SetBackgroundColour(wxColour(200, 100, 50));

        wxBoxSizer* keyboardContainerSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* keysSizer = new wxBoxSizer(wxHORIZONTAL);

        keysSizer->Add(CreateKeyColumn(keyboardPanel, {"1", "2", "3"}), 1, wxEXPAND | wxALL, 5);
        keysSizer->Add(CreateKeyColumn(keyboardPanel, {"4", "5", "6"}), 1, wxEXPAND | wxALL, 5);
        keysSizer->Add(CreateKeyColumn(keyboardPanel, {"7", "8", "9"}), 1, wxEXPAND | wxALL, 5);
        keysSizer->Add(CreateKeyColumn(keyboardPanel, {"0", "<<", ">>", "Enter"}), 1, wxEXPAND | wxALL, 5);

        keyboardContainerSizer->Add(keysSizer, 1, wxEXPAND | wxALL, 10);
        keyboardPanel->SetSizer(keyboardContainerSizer);

        mainSizer->Add(keyboardPanel, 0, wxEXPAND);

        mainPanel->SetSizer(mainSizer);
        Layout();
    }

public:
    void SetRevealText(const wxString& txt) {
        if (m_revealText) m_revealText->SetLabel(txt);
    }

private:
    wxStaticText* m_revealText = nullptr;
private:
    wxBoxSizer* CreateRow(wxPanel* parent, bool enabled) {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        wxSize inputSize(40, 40);
        long style = wxTE_CENTER;

        wxArrayString books;
        wxChoice* bookChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(80, 40), books);
        bookChoice->Enable(enabled);
        if(enabled && !books.IsEmpty()) bookChoice->SetSelection(0);
        sizer->Add(bookChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxTextCtrl* ch1 = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, inputSize, style);
        ch1->Enable(enabled);
        sizer->Add(ch1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxTextCtrl* ch2 = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, inputSize, style);
        ch2->Enable(enabled);
        sizer->Add(ch2, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxStaticText* sep = new wxStaticText(parent, wxID_ANY, ":");
        wxFont font = sep->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        sep->SetFont(font);
        sizer->Add(sep, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxTextCtrl* v1 = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, inputSize, style);
        v1->Enable(enabled);
        sizer->Add(v1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxTextCtrl* v2 = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, inputSize, style);
        v2->Enable(enabled);
        sizer->Add(v2, 0, wxALIGN_CENTER_VERTICAL);

        return sizer;
    }

    wxBoxSizer* CreateKeyColumn(wxPanel* parent, const std::vector<wxString>& labels) {
        wxBoxSizer* colSizer = new wxBoxSizer(wxVERTICAL);
        wxColour keyBg(235, 230, 157);
        wxColour keyFg(200, 100, 50);

        for (const auto& label : labels) {
            wxButton* btn = new wxButton(parent, wxID_ANY, label);
            btn->SetBackgroundColour(keyBg);
            btn->SetForegroundColour(keyFg);

            wxFont font = btn->GetFont();
            font.SetWeight(wxFONTWEIGHT_BOLD);
            btn->SetFont(font);

            colSizer->Add(btn, 1, wxEXPAND | wxBOTTOM, 5);
        }
        return colSizer;
    }
};