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
    std::vector<std::string> getAllBooks() const;
    std::string getRevealedText(const Verse& verse, int stage);
    std::string getBookArea(const std::string& bookName) const;
    bool isLoaded() const { return !m_verses.empty(); }

private:
    std::vector<Verse> m_verses;
    int calculateSliceSteps(int listLength, int chunkSize);
    std::vector<std::vector<std::string>> sliceList(const std::vector<std::string>& list, int chunkSize);
};
