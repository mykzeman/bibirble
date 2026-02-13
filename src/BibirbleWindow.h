#pragma once

#include <wx/wx.h>
#include "BibleData.h"
#include "GameRow.h"

class BibirbleWindow : public wxFrame {
public:
    explicit BibirbleWindow(wxWindow* parent, const std::string& dataPath = "");
    
private:
    void SetupUi();
    void SetupKeyboard(wxBoxSizer* mainLayout);
    void StartNewGame();
    void UpdateRevealText();
    void HandleKeyPress(const wxString& key);
    void FocusNext();
    void FocusPrev();
    int ProcessTurn();
    
    void OnVirtualKeyClicked(wxCommandEvent& event);
    void OnSubmit(wxCommandEvent& event);
    void OnShare(wxCommandEvent& event);
    
    BibleData m_data;
    Verse m_targetVerse;
    int m_currentStage = 0;
    bool m_gameOver = false;
    
    wxPanel* m_centralPanel;
    wxStaticText* m_revealPanel;
    wxPanel* m_rowsPanel;
    wxBoxSizer* m_rowsSizer;
    wxVector<GameRow*> m_rows;
    wxButton* m_submitBtn;
    
    wxWindow* m_focusedInput;
    
    wxDECLARE_EVENT_TABLE();
};
