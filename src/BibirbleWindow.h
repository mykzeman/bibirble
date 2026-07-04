#pragma once

#include <wx/wx.h>
#include "BibleData.h"
#include "GameRow.h"
#include "GameState.h"
#include "PersistenceManager.h"
#include "SettingsScreen.h"
#include "StartScreen.h"

class BibirbleWindow : public wxFrame {
public:
    explicit BibirbleWindow(wxWindow* parent, const std::string& dataPath = "");

private:
    void SetupUi();
    void SetupGamePanel(wxBoxSizer* mainLayout);
    void SetupKeyboard(wxBoxSizer* parentLayout);
    void SetupPostGameControls(wxBoxSizer* mainLayout);

    void ShowStartScreen();
    void ShowSettingsScreen();
    void ShowGamePanel();

    void StartNewGame(GameMode mode, bool hardMode, const wxString& seedText);
    void ResetGameUi();
    void UpdateRevealText();
    void HandleKeyPress(const wxString& key);
    void FocusNext();
    void FocusPrev();
    int ProcessTurn();

    void OnStartDaily();
    void OnStartRandom();
    void OnOpenSettings();
    void OnSettingsBack();
    void OnRandomizeSeed();

    void OnMainMenu();
    void OnPlayRandom();
    void OnViewSetSeed();

    void OnVirtualKeyClicked(wxCommandEvent& event);
    void OnSubmit(wxCommandEvent& event);
    void OnShare(wxCommandEvent& event);

    BibleData m_data;
    GameState m_state;
    PersistenceManager m_persistence;

    enum class ReturnScreen { Menu, Game };
    ReturnScreen m_settingsReturnTo = ReturnScreen::Menu;

    wxPanel* m_centralPanel;
    wxBoxSizer* m_mainLayout;

    StartScreen* m_startScreen;
    SettingsScreen* m_settingsScreen;
    wxPanel* m_gamePanel;

    wxStaticText* m_revealPanel;
    wxPanel* m_postGameControls;

    wxPanel* m_rowsPanel;
    wxBoxSizer* m_rowsSizer;
    wxVector<GameRow*> m_rows;
    wxButton* m_submitBtn;

    wxWindow* m_focusedInput;

    wxDECLARE_EVENT_TABLE();
};
