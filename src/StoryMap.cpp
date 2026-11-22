#include "StoryMap.h"

#include "EmbeddedData.h"

#include <algorithm>
#include <array>
#include <map>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace {

std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string Trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

bool ParseTiers(const json& j, GuessTiers& out) {
    if (!j.is_object()) return false;
    try {
        out.tier3 = j.at("3").get<int>();
        out.tier2 = j.at("2").get<int>();
        out.tier1 = j.at("1").get<int>();
    } catch (...) {
        return false;
    }
    return out.tier3 <= out.tier2 && out.tier2 <= out.tier1;
}

GuessTiers ScaleTiers(const GuessTiers& t, double scale) {
    GuessTiers out;
    out.tier3 = static_cast<int>(std::ceil(t.tier3 * scale));
    out.tier2 = static_cast<int>(std::ceil(t.tier2 * scale));
    out.tier1 = static_cast<int>(std::ceil(t.tier1 * scale));
    return out;
}

}  // namespace

int StarFormula::Eval(int x) const {
    return std::max(0, a * x + b);
}

bool StarFormula::Parse(const json& value, StarFormula& out) {
    out = StarFormula{};
    if (value.is_null()) return true;
    if (value.is_number_integer()) {
        out.b = value.get<int>();
        return true;
    }
    if (!value.is_string()) return false;
    static const std::regex kNumber(R"(^\s*(-?\d+)\s*$)");
    static const std::regex kFormula(R"(^\s*(\d*)\s*x\s*(?:([+-])\s*(\d+))?\s*$)");
    const std::string s = Lower(value.get<std::string>());
    std::smatch m;
    if (std::regex_match(s, m, kNumber)) {
        out.b = std::stoi(m[1].str());
        return true;
    }
    if (std::regex_match(s, m, kFormula)) {
        out.a = m[1].str().empty() ? 1 : std::stoi(m[1].str());
        if (m[3].matched) out.b = std::stoi(m[3].str()) * (m[2].str() == "-" ? -1 : 1);
        return true;
    }
    return false;
}

int GuessTiers::StarsFor(int totalGuesses) const {
    if (totalGuesses <= tier3) return 3;
    if (totalGuesses <= tier2) return 2;
    if (totalGuesses <= tier1) return 1;
    return 0;
}

bool StoryLevel::Allows(BonusStar star) const {
    switch (star) {
        case BonusStar::Hard: return allowHard;
        case BonusStar::FirstTry: return allowFirstTry;
        case BonusStar::NoRetry: return allowNoRetry;
        case BonusStar::Perfect: return allowPerfect;
    }
    return false;
}

int StoryLevel::MaxStars() const {
    return 3 + (allowHard ? 1 : 0) + (allowFirstTry ? 1 : 0) + (allowNoRetry ? 1 : 0) + (allowPerfect ? 1 : 0);
}

namespace {
// book -> (chapter, verse, BibleData index) for every verse, built once per
// load so expanding ~200 pool references stays fast. Pools are written in
// World English Bible numbering, so verses go in by their standard reference
// (the Douay-Rheims' Psalm 22 is filed under psalms 23).
using BookIndex = std::map<std::string, std::vector<std::array<int, 3>>>;

BookIndex BuildBookIndex(const BibleData& bible) {
    BookIndex index;
    const int count = bible.getVerseCount();
    for (int i = 0; i < count; ++i) {
        const Verse v = bible.getVerseAtIndex(i);
        if (v.hasStdRef) index[v.book].push_back({v.stdChapter, v.stdVerse, i});
    }
    return index;
}

bool ExpandPoolRefIndexed(const std::string& ref, const BookIndex& index,
                          std::vector<int>& out, std::string& error) {
    static const std::regex kRef(R"(^\s*(\w+)(?:\s+(\d+)(?:-(\d+))?(?::(\d+)(?:-(\d+))?)?)?\s*$)");
    std::smatch m;
    const std::string lower = Lower(ref);
    if (!std::regex_match(lower, m, kRef)) {
        error = "can't read pool reference \"" + ref + "\"";
        return false;
    }
    const std::string book = m[1].str();
    const bool hasChapter = m[2].matched;
    const int c1 = hasChapter ? std::stoi(m[2].str()) : 0;
    const int c2 = m[3].matched ? std::stoi(m[3].str()) : c1;
    const bool hasVerse = m[4].matched;
    const int v1 = hasVerse ? std::stoi(m[4].str()) : 0;
    const int v2 = m[5].matched ? std::stoi(m[5].str()) : v1;

    const auto it = index.find(book);
    if (it == index.end()) {
        error = "unknown book \"" + book + "\" in \"" + ref + "\"";
        return false;
    }
    const size_t before = out.size();
    for (const auto& [chapter, verse, i] : it->second) {
        bool ok;
        if (!hasChapter) {
            ok = true;
        } else if (hasVerse) {
            ok = chapter == c1 && verse >= v1 && verse <= v2;
        } else {
            ok = chapter >= c1 && chapter <= c2;
        }
        if (ok) out.push_back(i);
    }
    if (out.size() == before) {
        error = "\"" + ref + "\" matches no verses";
        return false;
    }
    return true;
}

// True if ref is written like a pool reference (it may still match nothing).
bool ReadsAsRef(const std::string& ref) {
    static const std::regex kRef(R"(^\s*(\w+)(?:\s+(\d+)(?:-(\d+))?(?::(\d+)(?:-(\d+))?)?)?\s*$)");
    return std::regex_match(Lower(ref), kRef);
}

// Set by LoadFromText for the duration of a load.
const BookIndex* g_loadIndex = nullptr;
}  // namespace

bool ExpandPoolRef(const std::string& ref, const BibleData& bible,
                   std::vector<int>& out, std::string& error) {
    if (g_loadIndex) return ExpandPoolRefIndexed(ref, *g_loadIndex, out, error);
    const BookIndex index = BuildBookIndex(bible);
    return ExpandPoolRefIndexed(ref, index, out, error);
}

bool StoryMap::ParseLevel(const json& j, const std::string& areaId, bool isBonus,
                          const GuessTiers& defaultTiers, double hardScale,
                          const BibleData& bible, StoryLevel& out) {
    out = StoryLevel{};
    out.id = j.value("id", "");
    out.areaId = areaId;
    out.key = areaId + "." + out.id;
    out.title = j.value("title", out.id);
    out.isBonus = isBonus;
    bool ok = true;
    auto fail = [&](const std::string& msg) {
        m_errors.push_back(out.key + ": " + msg);
        ok = false;
    };
    auto parseStory = [&](const json& value, const char* where, StoryText& into) {
        std::string err;
        if (!value.is_string()) {
            if (!value.is_null()) fail(std::string(where) + " must be text");
            return;
        }
        if (!StoryText::Parse(value.get<std::string>(), into, err)) fail(std::string(where) + ": " + err);
    };
    parseStory(j.value("intro", json()), "intro", out.intro);
    parseStory(j.value("outro", json()), "outro", out.outro);
    if (j.contains("between")) {
        if (!j["between"].is_array() || j["between"].size() > 2) {
            fail("between must be a list of up to 2 texts (after verse 1, after verse 2)");
        } else {
            for (const auto& b : j["between"]) {
                out.between.emplace_back();
                parseStory(b, "between", out.between.back());
            }
        }
    }
    if (out.id.empty()) fail("level has no id");
    out.region = j.value("region", "");
    if (out.region.empty()) fail("level has no region (where it sits on the map)");
    else if (!FindRegion(out.region)) fail("unknown region \"" + out.region + "\"");

    std::vector<std::string> refs;
    if (j.contains("pool") && j["pool"].is_string()) {
        refs.push_back(j["pool"].get<std::string>());
    } else if (j.contains("pool") && j["pool"].is_array()) {
        for (const auto& r : j["pool"]) {
            if (r.is_string()) refs.push_back(r.get<std::string>());
        }
    }
    if (refs.empty()) fail("level has no pool");
    // The map is written against the World English Bible, which must have
    // every passage (tools/check_story_map.py). Another version may lack one
    // (the Douay-Rheims source has no Malachi 4): it's skipped there, as long
    // as the level still has 3+ verses.
    const bool strict = std::string(bible.version().id) == "web";
    for (const auto& r : refs) {
        std::string err;
        if (!ExpandPoolRef(r, bible, out.pool, err) && (strict || !ReadsAsRef(r))) fail(err);
    }
    std::sort(out.pool.begin(), out.pool.end());
    out.pool.erase(std::unique(out.pool.begin(), out.pool.end()), out.pool.end());
    if (ok && out.pool.size() < 3) {
        // Partial Bibles (Tyndale's New Testament, the Septuagints...) just
        // can't play this level; the World English Bible must have it all.
        if (strict) fail("pool has only " + std::to_string(out.pool.size()) + " verses (needs 3+)");
        else out.available = false;
    }

    if (!StarFormula::Parse(j.value("minStars", json()), out.minStars)) fail("can't read minStars");
    if (!StarFormula::Parse(j.value("requiresStars", json()), out.requiresStars)) fail("can't read requiresStars");

    out.guessTiers = defaultTiers;
    if (j.contains("guessStars") && !ParseTiers(j["guessStars"], out.guessTiers)) {
        fail("guessStars needs increasing \"3\", \"2\", \"1\" limits");
    }
    out.hardGuessTiers = ScaleTiers(out.guessTiers, hardScale);
    if (j.contains("hardGuessStars") && !ParseTiers(j["hardGuessStars"], out.hardGuessTiers)) {
        fail("hardGuessStars needs increasing \"3\", \"2\", \"1\" limits");
    }

    if (j.contains("bonus") && j["bonus"].is_array()) {
        out.allowHard = out.allowFirstTry = out.allowNoRetry = out.allowPerfect = false;
        for (const auto& b : j["bonus"]) {
            const std::string name = b.is_string() ? b.get<std::string>() : "";
            if (name == "hard") out.allowHard = true;
            else if (name == "firstTry") out.allowFirstTry = true;
            else if (name == "noRetry") out.allowNoRetry = true;
            else if (name == "perfect") out.allowPerfect = true;
            else fail("unknown bonus star \"" + name + "\"");
        }
    }
    return ok;
}

bool StoryMap::LoadFromText(const std::string& text, const BibleData& bible) {
    m_areas.clear();
    m_regions.clear();
    m_errors.clear();
    m_loaded = false;

    json root;
    try {
        root = json::parse(text);
    } catch (const std::exception& e) {
        m_errors.push_back(std::string("story_map.json isn't valid JSON: ") + e.what());
        return false;
    }
    if (!root.is_object() || !root.contains("areas") || !root["areas"].is_array()) {
        m_errors.push_back("story_map.json needs an \"areas\" list");
        return false;
    }

    const BookIndex bookIndex = BuildBookIndex(bible);
    g_loadIndex = &bookIndex;
    struct ClearIndex { ~ClearIndex() { g_loadIndex = nullptr; } } clearIndex;

    // Places on the map picture; every level names one.
    if (root.contains("regions") && root["regions"].is_array()) {
        std::set<std::string> regionIds;
        for (const auto& r : root["regions"]) {
            StoryRegion region;
            if (r.is_object()) {
                region.id = r.value("id", "");
                region.name = r.value("name", region.id);
                region.offMap = r.value("offMap", false);
                region.approximate = r.value("approximate", false);
                region.x = r.value("x", region.offMap ? 0.0 : -1.0);
                region.y = r.value("y", region.offMap ? 0.0 : -1.0);
            }
            if (region.id.empty()) {
                m_errors.push_back("a region has no id");
            } else if (!regionIds.insert(region.id).second) {
                m_errors.push_back("duplicate region id \"" + region.id + "\"");
            } else if (region.x < 0 || region.x > 100 || region.y < 0 || region.y > 100) {
                m_errors.push_back("region \"" + region.id + "\" needs x and y between 0 and 100");
            } else {
                m_regions.push_back(region);
            }
        }
    } else {
        m_errors.push_back("story_map.json needs a \"regions\" list");
    }
    m_closeUp = StoryCloseUp{};
    if (root.contains("closeUp") && root["closeUp"].is_object()) {
        const json& c = root["closeUp"];
        m_closeUp.name = c.value("name", "Close-up");
        m_closeUp.x = c.value("x", 0.0);
        m_closeUp.y = c.value("y", 0.0);
        m_closeUp.size = c.value("size", 0.0);
        m_closeUp.pinX = c.value("pinX", m_closeUp.x + m_closeUp.size / 2);
        m_closeUp.pinY = c.value("pinY", m_closeUp.y + m_closeUp.size / 2);
        if (m_closeUp.size <= 0 || m_closeUp.x < 0 || m_closeUp.y < 0 ||
            m_closeUp.x + m_closeUp.size > 100 || m_closeUp.y + m_closeUp.size > 100) {
            m_errors.push_back("closeUp needs x, y and size inside the picture (percentages)");
            m_closeUp = StoryCloseUp{};
        }
    }

    GuessTiers defaultTiers;
    double hardScale = 1.25;
    if (root.contains("defaults") && root["defaults"].is_object()) {
        const json& d = root["defaults"];
        if (d.contains("guessStars") && !ParseTiers(d["guessStars"], defaultTiers)) {
            m_errors.push_back("defaults.guessStars needs increasing \"3\", \"2\", \"1\" limits");
        }
        if (d.contains("hardModeScale") && d["hardModeScale"].is_number()) {
            hardScale = d["hardModeScale"].get<double>();
        }
    }

    std::set<std::string> areaIds;
    for (const auto& a : root["areas"]) {
        StoryArea area;
        area.id = a.value("id", "");
        area.title = a.value("title", area.id);
        area.intro = a.value("intro", "");
        area.openFromStart = a.value("openFromStart", false);
        area.bonusArea = a.value("bonusArea", false);
        area.allBooks = a.value("allBooks", false);
        if (area.id.empty()) {
            m_errors.push_back("an area has no id");
            continue;
        }
        if (!areaIds.insert(area.id).second) m_errors.push_back("duplicate area id \"" + area.id + "\"");
        if (area.bonusArea && a.value("unlock", "") != "perfect") {
            m_errors.push_back(area.id + ": bonus area needs \"unlock\": \"perfect\"");
        }

        std::set<std::string> levelIds;
        auto parseList = [&](const char* field, bool isBonus, std::vector<StoryLevel>& into) {
            if (!a.contains(field) || !a[field].is_array()) return;
            for (const auto& lj : a[field]) {
                StoryLevel lvl;
                if (!lj.is_object()) {
                    m_errors.push_back(area.id + ": every level must be an object with an id and pool");
                    continue;
                }
                ParseLevel(lj, area.id, isBonus, defaultTiers, hardScale, bible, lvl);
                if (!lvl.id.empty() && !levelIds.insert(lvl.id).second) {
                    m_errors.push_back(lvl.key + ": duplicate level id");
                }
                into.push_back(std::move(lvl));
            }
        };
        parseList("levels", false, area.levels);
        parseList("bonusLevels", true, area.bonusLevels);
        if (area.levels.empty()) m_errors.push_back(area.id + ": area has no levels");
        m_areas.push_back(std::move(area));
    }
    if (m_areas.empty()) m_errors.push_back("story_map.json has no areas");
    if (!m_areas.empty() && (m_areas.front().bonusArea)) {
        m_errors.push_back("the first area can't be a bonus area");
    }

    m_loaded = m_errors.empty();
    return m_loaded;
}

bool StoryMap::Load(const BibleData& bible) {
    std::string text;
    if (!ReadStoryMapText(text)) {
        m_areas.clear();
        m_errors = {"Couldn't find story_map.json."};
        m_loaded = false;
        return false;
    }
    return LoadFromText(text, bible);
}

const StoryLevel* StoryMap::FindLevel(const std::string& key) const {
    for (const auto& area : m_areas) {
        for (const auto& l : area.levels) if (l.key == key) return &l;
        for (const auto& l : area.bonusLevels) if (l.key == key) return &l;
    }
    return nullptr;
}

const StoryRegion* StoryMap::FindRegion(const std::string& id) const {
    for (const auto& r : m_regions) if (r.id == id) return &r;
    return nullptr;
}

const StoryArea* StoryMap::FindArea(const std::string& id) const {
    for (const auto& area : m_areas) if (area.id == id) return &area;
    return nullptr;
}

std::vector<std::vector<std::string>> StoryMap::Paths() const {
    std::vector<std::vector<std::string>> paths;
    bool inPath = false;
    for (size_t i = 0; i < m_areas.size(); ++i) {
        const StoryArea& area = m_areas[i];
        if (area.bonusArea) {
            inPath = false;
            continue;
        }
        if (i == 0 || area.openFromStart || !inPath) {
            paths.emplace_back();
            inPath = true;
        }
        for (const auto& l : area.levels) paths.back().push_back(l.key);
    }
    return paths;
}

const StoryText* StoryLevel::StoryBefore(int verseIndex) const {
    if (verseIndex == 0) return &intro;
    if (verseIndex >= 1 && verseIndex <= (int)between.size()) return &between[verseIndex - 1];
    return nullptr;
}

std::vector<std::string> StoryMap::AllAnswers() const {
    std::vector<std::string> out;
    auto add = [&](const StoryText& t) { for (const auto& b : t.blanks) out.push_back(b.answer); };
    auto addLevel = [&](const StoryLevel& l) {
        add(l.intro);
        add(l.outro);
        for (const auto& b : l.between) add(b);
    };
    for (const auto& area : m_areas) {
        for (const auto& l : area.levels) addLevel(l);
        for (const auto& l : area.bonusLevels) addLevel(l);
    }
    return out;
}
