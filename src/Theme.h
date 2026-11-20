#pragma once

#include <wx/colour.h>

#include "GuessColor.h"
#include "SeededRandom.h"

// App colours. All through December they switch to a Christmas look like the
// web version: snowy ivory, with Christmas red buttons and keys. The clue
// colours on the guess boxes never change.
namespace Theme {

// December on the player's own calendar (the same local day the Daily uses),
// checked once when the app starts.
inline bool Christmas() {
    static const bool on = IsChristmasSeason();
    return on;
}

inline wxColour Background() {
    return Christmas() ? wxColour(247, 242, 228) : wxColour(234, 229, 159);
}

// Buttons, keys and the keyboard panel.
inline wxColour Accent() {
    return Christmas() ? wxColour(179, 38, 44) : wxColour(200, 100, 50);
}

// Text on Accent().
inline wxColour AccentText() {
    return Christmas() ? wxColour(247, 242, 228) : wxColour(235, 230, 157);
}

// Clue colours, the same ones the guess rows use (GameRow::setDigitColors).
inline wxColour ClueColour(GuessColor c) {
    switch (c) {
        case GuessColor::Green: return *wxGREEN;
        case GuessColor::Yellow: return *wxYELLOW;
        case GuessColor::Gray:
        default: return *wxLIGHT_GREY;
    }
}

}  // namespace Theme
