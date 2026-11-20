#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

#include "BibleVersions.h"

using json = nlohmann::json;

struct Verse {
    std::string testament;
    std::string area;
    std::string book;
    int chapter;
    int verse;
    std::string text;
    bool mature = false;  // R18 content: hidden unless R18 mode is on
    // Where this verse sits in the World English Bible's numbering (the
    // "ref" field tools/sort.py writes when a version numbers it differently,
    // like the Douay-Rheims' Psalm 22 = Psalm 23). Story Mode, Useful verses
    // and Christmas verses find verses by it, so they pick the same passage
    // in every version. hasStdRef is false for verses with no World English
    // Bible match (Daniel 3:24-90 in the Douay-Rheims).
    int stdChapter = 0;
    int stdVerse = 0;
    bool hasStdRef = true;
};


class BibleData {
public:
    BibleData();
    // Loads one Bible version (src/BibleVersions.h): its file on disk if
    // there is one, otherwise the copy compiled into the program.
    bool loadVersion(const BibleVersion& version);
    // Loads a verse file from this exact path (tests).
    bool loadData(const std::string& filePath);
    // The first of a few likely folders (next to the program, build/,
    // Release/...) holding this file name, or "" if none does.
    static std::string ResolveDataFilePath(const std::string& fileName = "bible_sections.json");
    const BibleVersion& version() const { return *m_version; }
    // Dataset index of the verse at this World English Bible reference, or -1.
    int findStdRef(const std::string& book, int chapter, int verse) const;
    Verse getRandomVerse() const;
    Verse getVerseAtIndex(int index) const;
    int getVerseCount() const { return static_cast<int>(m_verses.size()); }
    bool verseExists(const std::string& book, int chapter, int verse) const;
    // Dataset indices of the curated "Useful verses only" pool (see
    // UsefulVerses.h), in dataset order so a seed maps to the same verse.
    std::vector<int> getUsefulVerseIndices() const;
    // Dataset indices of the Christmas verses (see ChristmasVerses.h), in
    // that list's order (not dataset order) so the December Daily matches
    // the web version.
    std::vector<int> getChristmasVerseIndices() const;
    // Verses a new game can pick from: the Useful pool (or every verse),
    // minus mature (R18) verses unless allowMature is set.
    std::vector<int> getCandidateIndices(bool usefulOnly, bool allowMature) const;
    // Books in the dataset, in traditional order (Genesis to Revelation).
    std::vector<std::string> getAllBooks() const;
    // Books that have at least one of the given verses (e.g. a game's
    // candidate pool), in traditional order.
    std::vector<std::string> getBooks(const std::vector<int>& indices) const;
    std::string getRevealedText(const Verse& verse, int stage);
    std::string getBookArea(const std::string& bookName) const;
    // Tidy name to show players ("1samuel" -> "1 Samuel", "songofsolomon" ->
    // "Song of Solomon"). Game logic keeps using the dataset keys.
    static std::string BookDisplayName(const std::string& book);
    // The dataset key for a tidy name (the reverse of BookDisplayName).
    static std::string BookKeyFromDisplayName(const std::string& name);
    bool isLoaded() const { return !m_verses.empty(); }

private:
    bool parse(const std::string& text);

    std::vector<Verse> m_verses;
    std::map<std::string, int> m_stdIndex;  // "book chapter:verse" (World English Bible numbering) -> index
    const BibleVersion* m_version = &BibleVersions().front();
    int calculateSliceSteps(int listLength, int chunkSize);
    // Contiguous [start, end) index ranges a list of the given length would be
    // chunked into. Working with index ranges (rather than re-finding words by
    // value) is what lets getRevealedText() reveal the correct occurrence of a
    // word that appears more than once in a verse.
    std::vector<std::pair<int, int>> sliceIndices(int listLength, int chunkSize);
};
