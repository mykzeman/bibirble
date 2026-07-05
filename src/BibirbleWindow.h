#pragma once

#include <wx/wx.h>
#include "BibleData.h"
#include "GameRow.h"
#include "GameState.h"
#include "PersistenceManager.h"
#include "SettingsScreen.h"
#include "StartScreen.h"
#include "StoryData.h"
#include "StoryInterstitialScreen.h"
#include "StoryMapScreen.h"
#include "StoryProgress.h"
#include "StorySectionScreen.h"

class BibirbleWindow : public wxFrame {
public:
    explicit BibirbleWindow(wxWindow* parent, const std::string& dataPath = "");

private:
    void SetupUi();
    void SetupGamePanel(wxBoxSizer* mainLayout);
    void SetupKeyboard(wxBoxSizer* parentLayout);
    void SetupPostGameControls(wxBoxSizer* mainLayout);
    void SetupStoryPostGameControls(wxBoxSizer* mainLayout);

    void ShowOnly(wxWindow* target);
    void ShowStartScreen();
    void ShowSettingsScreen();
    void ShowGamePanel();
    void ShowStoryMapScreen();
    void ShowStorySectionScreen();
    void ShowStoryInterstitialScreen();

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

    // --- Story Mode ---
    const StoryChapter& CurrentStoryChapter(const StorySection& section) const;
    void OnOpenStoryMode();
    void OnStoryMapBack();
    void OnStorySectionSelected(int sectionId);
    void OnStorySectionBack();
    void OnStoryChapterSelected(int chapterIndex);
    void BeginStoryChapterPlay();
    void StartStoryVerse();
    void HandleStoryRoundFinished();
    void FinishStoryChapter();
    void OnStoryContinueClicked();
    void OnStoryExitChapter();

    BibleData m_data;
    GameState m_state;
    PersistenceManager m_persistence;

    StoryData m_storyData;
    StoryProgress m_storyProgress;

    enum class ReturnScreen { Menu, Game };
    ReturnScreen m_settingsReturnTo = ReturnScreen::Menu;

    wxPanel* m_centralPanel;
    wxBoxSizer* m_mainLayout;

    StartScreen* m_startScreen;
    SettingsScreen* m_settingsScreen;
    wxPanel* m_gamePanel;

    StoryMapScreen* m_storyMapScreen;
    StorySectionScreen* m_storySectionScreen;
    StoryInterstitialScreen* m_storyInterstitialScreen;

    wxPanel* m_storyHeaderPanel;
    wxStaticText* m_storyHeaderText;

    wxStaticText* m_revealPanel;
    wxPanel* m_postGameControls;
    wxPanel* m_storyPostGameControls;
    wxButton* m_storyContinueBtn;

    wxPanel* m_rowsPanel;
    wxBoxSizer* m_rowsSizer;
    wxVector<GameRow*> m_rows;
    wxButton* m_submitBtn;

    wxWindow* m_focusedInput;

    bool m_lastRoundWon = false;

    // Active Story Mode play session.
    bool m_isStoryMode = false;
    int m_storySectionId = 0;
    int m_storyChapterIndex = 0;  // -1 selects the bonus prophecy chapter
    int m_storyVerseIndex = 0;
    int m_storyCorrectCount = 0;
    bool m_storyChapterPerfect = true;
    bool m_storyChapterJustFinished = false;

    wxDECLARE_EVENT_TABLE();
};
