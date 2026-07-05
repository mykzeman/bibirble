#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Verse {
    std::string testament;
    std::string area;
    std::string book;
    int chapter;
    int verse;
    std::string text;
};

class BibleData {
public:
    BibleData();
    bool loadData(const std::string& filePath);
    static std::string ResolveDataFilePath(const std::string& preferredPath = "");
    Verse getRandomVerse() const;
    Verse getVerseAtIndex(int index) const;
    int getVerseCount() const { return static_cast<int>(m_verses.size()); }
    bool verseExists(const std::string& book, int chapter, int verse) const;
    // Looks up the full Verse (with text) for a specific reference. Returns a
    // default-constructed Verse (empty book/text) if the reference isn't found.
    Verse getVerse(const std::string& book, int chapter, int verse) const;
    std::vector<std::string> getAllBooks() const;
    std::string getRevealedText(const Verse& verse, int stage);
    std::string getBookArea(const std::string& bookName) const;
    bool isLoaded() const { return !m_verses.empty(); }

private:
    std::vector<Verse> m_verses;
    int calculateSliceSteps(int listLength, int chunkSize);
    // Contiguous [start, end) index ranges a list of the given length would be
    // chunked into. Working with index ranges (rather than re-finding words by
    // value) is what lets getRevealedText() reveal the correct occurrence of a
    // word that appears more than once in a verse.
    std::vector<std::pair<int, int>> sliceIndices(int listLength, int chunkSize);
};
