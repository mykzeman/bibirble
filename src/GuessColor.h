#pragma once

// Single canonical result color, shared by GameRow (rendering) and GameState
// (guess history / share text). Having exactly one enum here is what keeps the
// web version's gray/grey string-literal mismatch from being able to recur.
enum class GuessColor { Gray, Yellow, Green };
