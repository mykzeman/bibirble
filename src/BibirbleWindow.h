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
#include "StoryScreen.h"

#include <map>
#include <random>

class BibirbleWindow : public wxFrame {
public:
    // preloaded: verses already loaded (by LoadingDialog) for the saved Bible
    // version; if it's empty, the window loads the version itself.
    BibirbleWindow(wxWindow* parent, BibleData preloaded = BibleData());

private:
    void SetupUi();
    void SetupGamePanel(wxBoxSizer* mainLayout);
    void SetupKeyboard(wxBoxSizer* parentLayout);
    void SetupPostGameControls(wxBoxSizer* mainLayout);

    void ShowStartScreen();
    void ShowSettingsScreen();
    void ShowGamePanel();
    void ShowOnly(wxWindow* screen);
    // Landscape (window wider than tall): the number keys go to the right of
    // the verse and guess rows instead of below them. Checked when a game
    // starts and whenever the window is resized.
    void ApplyGameOrientation();
    // Colours the number keys by the best clue each digit has had this game.
    void UpdateDigitKeyColours();

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
    void ContinueStoryRun();
    void StartStoryVerse();
    // The books a Story level's book list offers: those of its story area.
    std::vector<std::string> StoryAreaBooks(const std::string& levelKey) const;
    void HandleStoryVerseEnd(bool won);
    void UpdateStoryHud();
    void SaveStory();
    // Loads a Bible version (falling back to the World English Bible) and
    // rebuilds Story Mode's pools and progress for it.
    void LoadBibleVersion(const std::string& id);
    // Rebuilds what depends on the loaded verses (Story Mode pools/progress).
    void OnBibleDataLoaded();
    // Switches to the version picked in Settings, if it changed. Called
    // between games so a game never mixes two versions.
    void ApplyBibleVersionSetting();
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
    StoryScreen* m_storyScreen;  // story text + fill-in-the-blank puzzles, full screen
    wxPanel* m_gamePanel;

    StoryMap m_storyMap;
    StoryProgress m_story;
    WordList m_words;  // valid guesses for story fill-in-the-blank puzzles
    std::mt19937 m_rng{std::random_device{}()};  // Story draws are not seeded
    wxPanel* m_storyHud = nullptr;
    wxStaticText* m_storyHudText = nullptr;

    wxBoxSizer* m_gameLayout = nullptr;   // verse/rows column + keyboard; vertical or horizontal
    wxPanel* m_keyboardPanel = nullptr;
    std::map<std::string, wxButton*> m_digitKeys;  // "0".."9"
    bool m_landscape = false;

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
