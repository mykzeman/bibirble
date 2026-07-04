#include "GameState.h"

void GameState::Reset(GameMode newMode, int64_t newSeed, bool newHardMode, const Verse& verse) {
    mode = newMode;
    seed = newSeed;
    hardMode = newHardMode;
    currentStage = 0;
    gameOver = false;
    targetVerse = verse;
    history.clear();
}
