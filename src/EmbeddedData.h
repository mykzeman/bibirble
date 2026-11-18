#pragma once

#include <string>

// Reads bible_sections.json's contents. A file on disk (see
// BibleData::ResolveDataFilePath) wins, so a fresh tools/sort.py output can be
// tested without rebuilding; otherwise it falls back to a copy compiled into
// the program (Bibirble.rc on Windows, .incbin on Linux), so the game ships as
// a single file.
// Returns false (out left empty) if neither is available.
bool ReadBibleDataText(const std::string& preferredPath, std::string& out);
