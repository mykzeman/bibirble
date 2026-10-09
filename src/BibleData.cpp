#include "BibleData.h"
#include "UsefulVerses.h"
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
            v.mature = obj.value("mature", false);
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

Verse BibleData::getVerseAtIndex(int index) const {
    if (index < 0 || index >= (int)m_verses.size()) return Verse();
    return m_verses[index];
}

std::vector<int> BibleData::getUsefulVerseIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < (int)m_verses.size(); ++i) {
        const Verse& v = m_verses[i];
        for (const auto& ref : kUsefulVerses) {
            if (v.book == ref.book && v.chapter == ref.chapter && v.verse == ref.verse) {
                indices.push_back(i);
                break;
            }
        }
    }
    return indices;
}

std::vector<int> BibleData::getCandidateIndices(bool usefulOnly, bool allowMature) const {
    std::vector<int> pool;
    if (usefulOnly) pool = getUsefulVerseIndices();
    if (pool.empty()) {
        pool.resize(m_verses.size());
        for (int i = 0; i < (int)m_verses.size(); ++i) pool[i] = i;
    }
    if (allowMature) return pool;

    std::vector<int> candidates;
    for (int i : pool) {
        if (!m_verses[i].mature) candidates.push_back(i);
    }
    return candidates;
}

bool BibleData::verseExists(const std::string& book, int chapter, int verse) const {
    for (const auto& v : m_verses) {
        if (v.book == book && v.chapter == chapter && v.verse == verse) {
            return true;
        }
    }
    return false;
}

namespace {
// Traditional (Protestant canon) book order. Keep in sync with
// Bibirble-web's scripts/books.js.
const std::vector<std::string>& BookOrder() {
    static const std::vector<std::string> ORDER = {
        "genesis", "exodus", "leviticus", "numbers", "deuteronomy",
        "joshua", "judges", "ruth", "1samuel", "2samuel", "1kings", "2kings",
        "1chronicles", "2chronicles", "ezra", "nehemiah", "esther", "job",
        "psalms", "proverbs", "ecclesiastes", "songofsolomon", "isaiah",
        "jeremiah", "lamentations", "ezekiel", "daniel", "hosea", "joel", "amos",
        "obadiah", "jonah", "micah", "nahum", "habakkuk", "zephaniah", "haggai",
        "zechariah", "malachi",
        "matthew", "mark", "luke", "john", "acts", "romans", "1corinthians",
        "2corinthians", "galatians", "ephesians", "philippians", "colossians",
        "1thessalonians", "2thessalonians", "1timothy", "2timothy", "titus",
        "philemon", "hebrews", "james", "1peter", "2peter", "1john", "2john",
        "3john", "jude", "revelation"};
    return ORDER;
}

size_t BookRank(const std::string& book) {
    const auto& order = BookOrder();
    return std::find(order.begin(), order.end(), book) - order.begin();
}
}  // namespace

std::vector<std::string> BibleData::getAllBooks() const {
    std::vector<std::string> books;
    for (const auto& v : m_verses) {
        if (std::find(books.begin(), books.end(), v.book) == books.end()) {
            books.push_back(v.book);
        }
    }
    std::stable_sort(books.begin(), books.end(), [](const std::string& a, const std::string& b) {
        size_t ra = BookRank(a), rb = BookRank(b);
        return ra != rb ? ra < rb : a < b;
    });
    return books;
}

std::string BibleData::getBookArea(const std::string& bookName) const {
    static const std::map<std::string, std::vector<std::string>> AREAS = {
        {"Torah", {"genesis", "exodus", "leviticus", "numbers", "deuteronomy"}},
        {"Historical", {"joshua", "judges", "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles", "nehemiah"}},
        {"Poems", {"psalms", "proverbs", "ecclesiastes", "songofsolomon", "lamentations"}},
        {"Small stories", {"job", "esther", "jonah", "ruth", "ezra"}},
        {"Prophets Major", {"isaiah", "jeremiah", "ezekiel", "daniel"}},
        {"Prophets Minor", {"hosea", "joel", "amos", "obadiah", "micah", "nahum", "habakkuk", "zephaniah", "haggai", "zechariah", "malachi"}},
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

std::vector<std::pair<int, int>> BibleData::sliceIndices(int listLength, int chunkSize) {
    int size = std::max(1, chunkSize);
    int steps = calculateSliceSteps(listLength, size);
    std::vector<std::pair<int, int>> chunks;

    int start = 0;
    for (int i = 0; i < steps; i++) {
        int end = std::min(start + size, listLength);
        chunks.emplace_back(start, end);
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
    std::vector<std::pair<int, int>> chunks = sliceIndices(words.size(), slices);

    std::set<int> visibleIndices;

    auto revealChunk = [&](const std::pair<int, int>& range) {
        for (int i = range.first; i < range.second; ++i) {
            visibleIndices.insert(i);
        }
    };

    // Always show start chunk
    if (!chunks.empty()) {
        revealChunk(chunks[0]);
    }

    // Show chunks up to stage
    for (int i = 1; i <= stage; i++) {
        if (i >= (int)chunks.size()) break;
        revealChunk(chunks[i]);
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
