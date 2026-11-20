#pragma once

#include <string>

// Reads bible_sections.json's contents. A file on disk (see
// BibleData::ResolveDataFilePath) wins, so a fresh tools/sort.py output can be
// tested without rebuilding; otherwise it falls back to a copy compiled into
// the program (Bibirble.rc on Windows, .incbin on Linux), so the game ships as
// a single file.
// Returns false (out left empty) if neither is available.
bool ReadBibleDataText(const std::string& preferredPath, std::string& out);

// The app icon PNG (icons/bibirble.png) compiled into the Linux build, for the
// window/taskbar icon. Windows loads its icon from Bibirble.rc instead, so
// this returns false there (and on macOS).
bool ReadEmbeddedIconPng(std::string& out);

// Reads story_map.json's contents (Story Mode's map). Like the Bible data, a
// story_map.json on disk (next to the program, or in a parent/build folder)
// wins so map edits can be tested without rebuilding; otherwise the copy
// compiled into the program is used.
bool ReadStoryMapText(std::string& out);

// The English word list for story puzzles (data/words.txt, from SCOWL; see
// data/WORDLIST-LICENSE.txt). A copy on disk wins, like the others.
bool ReadWordListText(std::string& out);
