#include "BibleData.h"
#include "UsefulVerses.h"
#include "ChristmasVerses.h"
#include "EmbeddedData.h"
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

std::string BibleData::ResolveDataFilePath(const std::string& fileName) {
    for (const char* dir : {"", "./", "../", "Release/", "./Release/", "../Release/", "Debug/", "./Debug/",
                            "../Debug/", "build/", "./build/", "../build/", "build/Release/", "./build/Release/",
                            "../build/Release/", "build/Debug/", "./build/Debug/", "../build/Debug/"}) {
        const std::string candidate = dir + fileName;
        std::ifstream file(candidate, std::ios::binary);
        if (file.good()) {
            return candidate;
        }
    }
    return "";
}

bool BibleData::loadVersion(const BibleVersion& version) {
    std::string text;
    if (!ReadBibleDataText(version, text) || !parse(text)) {
        return false;
    }
    m_version = &version;
    return true;
}

bool BibleData::loadData(const std::string& filePath) {
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return parse(ss.str());
}

bool BibleData::parse(const std::string& text) {
    try {
        json arr = json::parse(text);
        
        if (!arr.is_array()) {
            return false;
        }
        
        std::vector<Verse> verses;
        std::map<std::string, int> stdIndex;
        for (const auto& obj : arr) {
            Verse v;
            v.testament = obj.value("testament", "");
            v.area = obj.value("area", "");
            v.book = obj.value("book", "");
            v.chapter = obj.value("chapter", 0);
            v.verse = obj.value("verse", 0);
            v.text = obj.value("text", "");
            v.mature = obj.value("mature", false);
            v.stdChapter = v.chapter;
            v.stdVerse = v.verse;
            if (obj.contains("ref")) {
                const json& ref = obj["ref"];
                if (ref.is_array() && ref.size() == 2) {
                    v.stdChapter = ref[0].get<int>();
                    v.stdVerse = ref[1].get<int>();
                } else {
                    v.hasStdRef = false;
                }
            }
            if (v.hasStdRef) {
                stdIndex.emplace(v.book + " " + std::to_string(v.stdChapter) + ":" + std::to_string(v.stdVerse),
                                 static_cast<int>(verses.size()));
            }
            verses.push_back(v);
        }
        if (verses.empty()) return false;
        m_verses = std::move(verses);
        m_stdIndex = std::move(stdIndex);
        return true;
    } catch (...) {
        return false;
    }
}

int BibleData::findStdRef(const std::string& book, int chapter, int verse) const {
    auto it = m_stdIndex.find(book + " " + std::to_string(chapter) + ":" + std::to_string(verse));
    return it == m_stdIndex.end() ? -1 : it->second;
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
    for (const auto& ref : kUsefulVerses) {
        const int i = findStdRef(ref.book, ref.chapter, ref.verse);
        if (i >= 0) indices.push_back(i);
    }
    std::sort(indices.begin(), indices.end());  // dataset order
    return indices;
}

std::vector<int> BibleData::getChristmasVerseIndices() const {
    std::vector<int> indices;
    for (const auto& ref : kChristmasVerses) {
        const int i = findStdRef(ref.book, ref.chapter, ref.verse);
        if (i >= 0) indices.push_back(i);
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
    // Useful verses only always stays family friendly: a verse picked from
    // the World English Bible can read as mature in another version.
    if (allowMature && !usefulOnly) return pool;

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
// Traditional book order, Genesis to Revelation. The deuterocanonical and
// apocryphal books (only some versions have them) sit near the books they
// belong with. Keep in sync with
// Bibirble-web's scripts/books.js.
const std::vector<std::string>& BookOrder() {
    static const std::vector<std::string> ORDER = {
        "genesis", "exodus", "leviticus", "numbers", "deuteronomy",
        "joshua", "judges", "ruth", "1samuel", "2samuel", "1kings", "2kings",
        "1chronicles", "2chronicles", "ezra", "nehemiah", "1esdras", "2esdras", "tobit", "judith",
        "esther", "esthergreek", "job", "psalms", "prayerofmanasseh", "proverbs",
        "ecclesiastes", "songofsolomon", "wisdom", "sirach", "psalmsofsolomon", "isaiah",
        "jeremiah", "lamentations", "baruch", "letterofjeremiah", "ezekiel", "daniel", "danielgreek",
        "songofthethree", "susanna", "bel", "hosea", "joel", "amos",
        "obadiah", "jonah", "micah", "nahum", "habakkuk", "zephaniah", "haggai",
        "zechariah", "malachi", "1maccabees", "2maccabees", "3maccabees", "4maccabees",
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

namespace {
// Tidy book names. Keep in sync with BOOK_NAMES in Bibirble-web's
// scripts/books.js.
const std::map<std::string, std::string>& BookNames() {
    static const std::map<std::string, std::string> NAMES = {
        {"genesis", "Genesis"}, {"exodus", "Exodus"}, {"leviticus", "Leviticus"},
        {"numbers", "Numbers"}, {"deuteronomy", "Deuteronomy"}, {"joshua", "Joshua"},
        {"judges", "Judges"}, {"ruth", "Ruth"}, {"1samuel", "1 Samuel"},
        {"2samuel", "2 Samuel"}, {"1kings", "1 Kings"}, {"2kings", "2 Kings"},
        {"1chronicles", "1 Chronicles"}, {"2chronicles", "2 Chronicles"},
        {"ezra", "Ezra"}, {"nehemiah", "Nehemiah"}, {"esther", "Esther"},
        {"job", "Job"}, {"psalms", "Psalms"}, {"proverbs", "Proverbs"},
        {"ecclesiastes", "Ecclesiastes"}, {"songofsolomon", "Song of Solomon"},
        {"isaiah", "Isaiah"}, {"jeremiah", "Jeremiah"},
        {"lamentations", "Lamentations"}, {"ezekiel", "Ezekiel"}, {"daniel", "Daniel"},
        {"hosea", "Hosea"}, {"joel", "Joel"}, {"amos", "Amos"}, {"obadiah", "Obadiah"},
        {"jonah", "Jonah"}, {"micah", "Micah"}, {"nahum", "Nahum"},
        {"habakkuk", "Habakkuk"}, {"zephaniah", "Zephaniah"}, {"haggai", "Haggai"},
        {"zechariah", "Zechariah"}, {"malachi", "Malachi"}, {"matthew", "Matthew"},
        {"mark", "Mark"}, {"luke", "Luke"}, {"john", "John"}, {"acts", "Acts"},
        {"romans", "Romans"}, {"1corinthians", "1 Corinthians"},
        {"2corinthians", "2 Corinthians"}, {"galatians", "Galatians"},
        {"ephesians", "Ephesians"}, {"philippians", "Philippians"},
        {"colossians", "Colossians"}, {"1thessalonians", "1 Thessalonians"},
        {"2thessalonians", "2 Thessalonians"}, {"1timothy", "1 Timothy"},
        {"2timothy", "2 Timothy"}, {"titus", "Titus"}, {"philemon", "Philemon"},
        {"hebrews", "Hebrews"}, {"james", "James"}, {"1peter", "1 Peter"},
        {"2peter", "2 Peter"}, {"1john", "1 John"}, {"2john", "2 John"},
        {"3john", "3 John"}, {"jude", "Jude"}, {"revelation", "Revelation"},
        {"tobit", "Tobit"}, {"judith", "Judith"}, {"wisdom", "Wisdom"}, {"sirach", "Sirach"},
        {"baruch", "Baruch"}, {"1maccabees", "1 Maccabees"}, {"2maccabees", "2 Maccabees"},
        {"3maccabees", "3 Maccabees"}, {"4maccabees", "4 Maccabees"}, {"1esdras", "1 Esdras"},
        {"2esdras", "2 Esdras"}, {"esthergreek", "Esther (Greek)"},
        {"prayerofmanasseh", "Prayer of Manasseh"}, {"psalmsofsolomon", "Psalms of Solomon"},
        {"letterofjeremiah", "Letter of Jeremiah"}, {"danielgreek", "Daniel (Greek)"},
        {"songofthethree", "Song of the Three"}, {"susanna", "Susanna"}, {"bel", "Bel and the Dragon"}
    };
    return NAMES;
}
}  // namespace

std::string BibleData::BookDisplayName(const std::string& book) {
    const auto& names = BookNames();
    auto it = names.find(book);
    return it != names.end() ? it->second : book;
}

std::string BibleData::BookKeyFromDisplayName(const std::string& name) {
    for (const auto& entry : BookNames()) {
        if (entry.second == name) return entry.first;
    }
    return name;
}

std::vector<std::string> BibleData::getAllBooks() const {
    std::vector<int> all(m_verses.size());
    for (int i = 0; i < (int)m_verses.size(); ++i) all[i] = i;
    return getBooks(all);
}

std::vector<std::string> BibleData::getBooks(const std::vector<int>& indices) const {
    std::vector<std::string> books;
    for (int i : indices) {
        if (i < 0 || i >= (int)m_verses.size()) continue;
        const std::string& book = m_verses[i].book;
        if (std::find(books.begin(), books.end(), book) == books.end()) {
            books.push_back(book);
        }
    }
    std::stable_sort(books.begin(), books.end(), [](const std::string& a, const std::string& b) {
        size_t ra = BookRank(a), rb = BookRank(b);
        return ra != rb ? ra < rb : a < b;
    });
    return books;
}

std::string BibleData::getBookArea(const std::string& bookName) const {
    // Must match AREAS in tools/sort.py and Bibirble-web's scripts/books.js.
    // New areas hold at most 8 books (only the original areas and Paul's
    // letters are bigger).
    static const std::map<std::string, std::vector<std::string>> AREAS = {
        {"Torah", {"genesis", "exodus", "leviticus", "numbers", "deuteronomy"}},
        {"Historical", {"joshua", "judges", "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles", "nehemiah", "1esdras"}},
        {"Poems", {"psalms", "proverbs", "ecclesiastes", "songofsolomon", "lamentations", "wisdom", "sirach", "prayerofmanasseh", "psalmsofsolomon"}},
        {"Small stories", {"job", "esther", "jonah", "ruth", "ezra", "tobit", "judith", "esthergreek"}},
        {"Prophets Major", {"isaiah", "jeremiah", "ezekiel", "daniel"}},
        {"Prophets Minor", {"hosea", "joel", "amos", "obadiah", "micah", "nahum", "habakkuk", "zephaniah", "haggai", "zechariah", "malachi"}},
        {"Prophet alternatives", {"baruch", "letterofjeremiah", "danielgreek", "songofthethree", "susanna", "bel", "2esdras"}},
        {"Maccabees", {"1maccabees", "2maccabees", "3maccabees", "4maccabees"}},
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
    // Round up, like data.js: the last, shorter chunk is still a stage.
    return (listLength + size - 1) / size;
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

std::string BibleData::getBookAuthor(const std::string& book) {
    // Hard mode groups books by who traditionally wrote them, so a yellow
    // book means "same author". Books whose writer isn't known share
    // "Unknown". Must match AUTHORS in Bibirble-web's scripts/books.js.
    static const std::map<std::string, std::vector<std::string>> AUTHORS = {
        {"Moses", {"genesis", "exodus", "leviticus", "numbers", "deuteronomy"}},
        {"Joshua", {"joshua"}},
        {"Samuel", {"judges", "ruth", "1samuel", "2samuel"}},
        {"Jeremiah", {"1kings", "2kings", "jeremiah", "lamentations", "letterofjeremiah"}},
        {"Ezra and Nehemiah", {"1chronicles", "2chronicles", "ezra", "nehemiah", "1esdras", "2esdras"}},
        {"David", {"psalms"}},
        {"Solomon", {"proverbs", "ecclesiastes", "songofsolomon", "wisdom"}},
        {"Ben Sira", {"sirach"}},
        {"Baruch", {"baruch"}},
        {"Isaiah", {"isaiah"}},
        {"Ezekiel", {"ezekiel"}},
        {"Daniel", {"daniel", "danielgreek"}},
        {"Hosea", {"hosea"}},
        {"Joel", {"joel"}},
        {"Amos", {"amos"}},
        {"Obadiah", {"obadiah"}},
        {"Jonah", {"jonah"}},
        {"Micah", {"micah"}},
        {"Nahum", {"nahum"}},
        {"Habakkuk", {"habakkuk"}},
        {"Zephaniah", {"zephaniah"}},
        {"Haggai", {"haggai"}},
        {"Zechariah", {"zechariah"}},
        {"Malachi", {"malachi"}},
        {"Matthew", {"matthew"}},
        {"Mark", {"mark"}},
        {"Luke", {"luke", "acts"}},
        {"John", {"john", "1john", "2john", "3john", "revelation"}},
        {"Paul", {"romans", "1corinthians", "2corinthians", "galatians", "ephesians", "philippians", "colossians", "1thessalonians", "2thessalonians", "1timothy", "2timothy", "titus", "philemon"}},
        {"Peter", {"1peter", "2peter"}},
        {"Jesus' brothers", {"james", "jude"}},
        {"Unknown", {"job", "esther", "hebrews", "tobit", "judith", "esthergreek", "songofthethree", "susanna", "bel",
                     "1maccabees", "2maccabees", "3maccabees", "4maccabees", "prayerofmanasseh", "psalmsofsolomon"}}
    };
    for (const auto& author : AUTHORS) {
        for (const auto& b : author.second) {
            if (b == book) return author.first;
        }
    }
    return "";
}
