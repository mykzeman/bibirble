#pragma once

#include <wx/wx.h>
#include "BibleData.h"
#include "GameRow.h"
#include "GameState.h"
#include "PersistenceManager.h"
#include "SettingsScreen.h"
#include "StartScreen.h"
#include "StoryMap.h"
#include "StoryMapScreen.h"
#include "StoryProgress.h"

#include <random>

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
    void ShowOnly(wxWindow* screen);

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

    // Story Mode (docs/STORY_MODE.md)
    void OnOpenStory();
    void ShowStoryMap();
    void OnStoryPlay(const std::string& levelKey, bool hardMode);
    void StartStoryVerse();
    void HandleStoryVerseEnd(bool won);
    void UpdateStoryHud();
    void SaveStory();
    bool AllowMature() const;

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
    StoryMapScreen* m_storyMapScreen;
    wxPanel* m_gamePanel;

    StoryMap m_storyMap;
    StoryProgress m_story;
    std::mt19937 m_rng{std::random_device{}()};  // Story draws are not seeded
    wxPanel* m_storyHud = nullptr;
    wxStaticText* m_storyHudText = nullptr;

    wxStaticText* m_revealPanel;
    wxPanel* m_postGameControls;

    wxPanel* m_rowsPanel;
    wxBoxSizer* m_rowsSizer;
    wxVector<GameRow*> m_rows;
    std::vector<std::string> m_allBooks;  // Books in the current game's pool, traditional order
    wxButton* m_submitBtn;

    wxWindow* m_focusedInput;

    wxDECLARE_EVENT_TABLE();
};
