#include "BibleData.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>
#include <set>
#include <string>
#include <map>
#include <vector>

BibleData::BibleData() {}

std::string BibleData::ResolveDataFilePath(const std::string& preferredPath) {
    std::vector<std::string> candidates;
    if (!preferredPath.empty()) {
        candidates.push_back(preferredPath);
    }

    const std::vector<std::string> defaults = {
        "bible_sections.json",
        "./bible_sections.json",
        "../bible_sections.json",
        "Release/bible_sections.json",
        "./Release/bible_sections.json",
        "../Release/bible_sections.json",
        "Debug/bible_sections.json",
        "./Debug/bible_sections.json",
        "../Debug/bible_sections.json",
        "build/bible_sections.json",
        "./build/bible_sections.json",
        "../build/bible_sections.json",
        "build/Release/bible_sections.json",
        "./build/Release/bible_sections.json",
        "../build/Release/bible_sections.json",
        "build/Debug/bible_sections.json",
        "./build/Debug/bible_sections.json",
        "../build/Debug/bible_sections.json"
    };

    candidates.insert(candidates.end(), defaults.begin(), defaults.end());

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate, std::ios::binary);
        if (file.good()) {
            return candidate;
        }
    }

    return "";
}

bool BibleData::loadData(const std::string& filePath) {
    const std::string resolvedPath = ResolveDataFilePath(filePath);
    if (resolvedPath.empty()) {
        return false;
    }

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json arr;
        file >> arr;
        
        if (!arr.is_array()) {
            return false;
        }
        
        m_verses.clear();
        for (const auto& obj : arr) {
            Verse v;
            v.testament = obj.value("testament", "");
            v.area = obj.value("area", "");
            v.book = obj.value("book", "");
            v.chapter = obj.value("chapter", 0);
            v.verse = obj.value("verse", 0);
            v.text = obj.value("text", "");
            m_verses.push_back(v);
        }
        return true;
    } catch (...) {
        return false;
    }
}

Verse BibleData::getRandomVerse() const {
    if (m_verses.empty()) return Verse();
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, m_verses.size() - 1);
    return m_verses[dis(gen)];
}

std::vector<std::string> BibleData::getAllBooks() const {
    std::vector<std::string> books;
    for (const auto& v : m_verses) {
        if (std::find(books.begin(), books.end(), v.book) == books.end()) {
            books.push_back(v.book);
        }
    }
    return books;
}

std::string BibleData::getBookArea(const std::string& bookName) const {
    static const std::map<std::string, std::vector<std::string>> AREAS = {
        {"Torah", {"genesis", "exodus", "leviticus", "numbers", "deuteronomy"}},
        {"Historical", {"joshua", "judges", "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles"}},
        {"Poems", {"psalms", "proverbs", "ecclesiastes", "songofsolomon", "lamentations"}},
        {"Small stories", {"job", "esther", "jonah", "ruth", "ezra"}},
        {"Prophets Major", {"isaiah", "jeremiah", "ezekiel", "daniel"}},
        {"Prophets Minor", {"hosea", "joel", "amos", "obadiah", "micah", "nahum", "habakkuk", "zephaniah", "haggai", "zechariah", "malachi", "nehemiah"}},
        {"Gospel", {"matthew", "mark", "luke", "john"}},
        {"Acts from Hebrews", {"acts", "hebrews"}},
        {"Pauls letters", {"romans", "1corinthians", "2corinthians", "galatians", "ephesians", "philippians", "colossians", "1thessalonians", "2thessalonians", "1timothy", "2timothy", "titus", "philemon"}},
        {"Peter letters", {"1peter", "2peter"}},
        {"James and Jude", {"james", "jude"}},
        {"John Letters and Visions", {"1john", "2john", "3john", "revelation"}}
    };

    std::string lowerBookName = bookName;
    std::transform(lowerBookName.begin(), lowerBookName.end(), lowerBookName.begin(), ::tolower);

    for (const auto& area : AREAS) {
        for (const auto& book : area.second) {
            if (book == lowerBookName) {
                return area.first;
            }
        }
    }
    return "";
}

int BibleData::calculateSliceSteps(int listLength, int chunkSize) {
    int size = std::max(1, chunkSize);
    return  std::ceil(listLength / size);
}

std::vector<std::vector<std::string>> BibleData::sliceList(const std::vector<std::string>& list, int chunkSize) {
    int len = list.size();
    int size = std::max(1, chunkSize);
    int steps = calculateSliceSteps(len, size);
    std::vector<std::vector<std::string>> chunks;
    
    int start = 0;
    for (int i = 0; i < steps; i++) {
        int end = std::min(start + size, len);
        std::vector<std::string> chunk(list.begin() + start, list.begin() + end);
        chunks.push_back(chunk);
        start = end;
    }
    return chunks;
}

std::string BibleData::getRevealedText(const Verse& verse, int stage) {
    if (stage == -1) return verse.text;

    // Split text into words
    std::vector<std::string> words;
    std::istringstream iss(verse.text);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }

    if (words.empty()) return "";

    // Logic from revealVerse in data.js
    int slices = std::floor(words.size() / 7);
    std::vector<std::vector<std::string>> chunks = sliceList(words, slices);
    
    std::set<int> visibleIndices;
    
    // Always show start chunk
    if (!chunks.empty()) {
        for (const auto& w : chunks[0]) {
            auto it = std::find(words.begin(), words.end(), w);
            if (it != words.end()) {
                visibleIndices.insert(std::distance(words.begin(), it));
            }
        }
    }

    // Show chunks up to stage
    for (int i = 1; i <= stage; i++) {
        if (i >= (int)chunks.size()) break;
        for (const auto& w : chunks[i]) {
            auto it = std::find(words.begin(), words.end(), w);
            if (it != words.end()) {
                visibleIndices.insert(std::distance(words.begin(), it));
            }
        }
    }

    // Build masked verse
    std::vector<std::string> maskedVerse;
    for (size_t i = 0; i < words.size(); ++i) {
        if (visibleIndices.count(i)) {
            maskedVerse.push_back(words[i]);
        } else {
            maskedVerse.push_back("...");
        }
    }

    std::string result;
    for (size_t i = 0; i < maskedVerse.size(); ++i) {
        if (i > 0) result += " ";
        result += maskedVerse[i];
    }
    return result;
}
