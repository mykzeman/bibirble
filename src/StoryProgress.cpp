#include "StoryProgress.h"

#include <algorithm>
#include <map>

namespace {

json StarsToJson(const LevelStars& s) {
    return json{{"guess", s.guess}, {"hard", s.hard}, {"firstTry", s.firstTry},
                {"noRetry", s.noRetry}, {"perfect", s.perfect}};
}

LevelStars StarsFromJson(const json& j) {
    LevelStars s;
    if (!j.is_object()) return s;
    s.guess = std::clamp(j.value("guess", 0), 0, 3);
    s.hard = j.value("hard", false);
    s.firstTry = j.value("firstTry", false);
    s.noRetry = j.value("noRetry", false);
    s.perfect = j.value("perfect", false);
    return s;
}

json GuessToJson(const GuessRecord& g) {
    std::vector<int> colors;
    for (GuessColor c : g.digitColors) colors.push_back(static_cast<int>(c));
    return json{{"book", g.bookGuess}, {"bookColor", static_cast<int>(g.bookColor)},
                {"digits", g.digitGuesses}, {"digitColors", colors}};
}

bool GuessFromJson(const json& j, GuessRecord& g) {
    try {
        g.bookGuess = j.at("book").get<std::string>();
        g.bookColor = static_cast<GuessColor>(j.at("bookColor").get<int>());
        g.digitGuesses = j.at("digits").get<std::vector<std::string>>();
        g.digitColors.clear();
        for (int c : j.at("digitColors").get<std::vector<int>>()) g.digitColors.push_back(static_cast<GuessColor>(c));
    } catch (...) {
        return false;
    }
    return g.digitGuesses.size() == 4 && g.digitColors.size() == 4;
}

}  // namespace

std::string VerseKey(const Verse& v) {
    // World English Bible numbering, so saved progress still finds its
    // verses after switching Bible versions.
    if (!v.hasStdRef) return v.book + " " + std::to_string(v.chapter) + ":" + std::to_string(v.verse) + "*";
    return v.book + " " + std::to_string(v.stdChapter) + ":" + std::to_string(v.stdVerse);
}

int LevelStars::Total() const {
    return guess + (hard ? 1 : 0) + (firstTry ? 1 : 0) + (noRetry ? 1 : 0) + (perfect ? 1 : 0);
}

LevelStars LevelStars::Merge(const LevelStars& o) const {
    LevelStars m;
    m.guess = std::max(guess, o.guess);
    m.hard = hard || o.hard;
    m.firstTry = firstTry || o.firstTry;
    m.noRetry = noRetry || o.noRetry;
    m.perfect = perfect || o.perfect;
    return m;
}

void StoryProgress::FromJson(const json& j, const BibleData& bible) {
    *this = StoryProgress{};
    if (!j.is_object()) return;

    std::map<std::string, int> indexOf;
    for (int i = 0; i < bible.getVerseCount(); ++i) indexOf[VerseKey(bible.getVerseAtIndex(i))] = i;
    auto toIndex = [&](const json& key) -> int {
        if (!key.is_string()) return -1;
        auto it = indexOf.find(key.get<std::string>());
        return it == indexOf.end() ? -1 : it->second;
    };

    if (j.contains("levels") && j["levels"].is_object()) {
        for (const auto& [key, lj] : j["levels"].items()) {
            LevelProgress p;
            p.passed = lj.value("passed", false);
            p.stars = StarsFromJson(lj.value("stars", json()));
            if (lj.contains("solvedVerses") && lj["solvedVerses"].is_array()) {
                for (const auto& v : lj["solvedVerses"]) {
                    int idx = toIndex(v);
                    if (idx >= 0) p.solved.insert(idx);
                }
            }
            p.retryPending = lj.value("retryPending", false);
            p.failedVerse = lj.contains("failedVerse") ? toIndex(lj["failedVerse"]) : -1;
            m_levels[key] = std::move(p);
        }
    }
    if (j.contains("unlocked") && j["unlocked"].is_array()) {
        for (const auto& k : j["unlocked"]) {
            if (k.is_string() && m_unlockedSet.insert(k.get<std::string>()).second) {
                m_unlocked.push_back(k.get<std::string>());
            }
        }
    }
    if (j.contains("inProgress") && j["inProgress"].is_object()) {
        const json& r = j["inProgress"];
        StoryRun run;
        run.levelKey = r.value("level", "");
        if (r.contains("verses") && r["verses"].is_array()) {
            for (const auto& v : r["verses"]) run.verses.push_back(toIndex(v));
        }
        run.solvedCount = r.value("solved", 0);
        run.totalGuesses = r.value("guessesSoFar", 0);
        run.hardMode = r.value("hardMode", false);
        run.anyFirstTry = r.value("anyFirstTry", false);
        run.storyDone = r.value("storyDone", true);
        if (r.contains("currentGuesses") && r["currentGuesses"].is_array()) {
            for (const auto& g : r["currentGuesses"]) {
                GuessRecord rec;
                if (GuessFromJson(g, rec)) run.currentGuesses.push_back(rec);
            }
        }
        const bool versesOk = run.verses.size() == kVersesPerLevel &&
            std::none_of(run.verses.begin(), run.verses.end(), [](int i) { return i < 0; });
        if (!run.levelKey.empty() && versesOk && run.solvedCount >= 0 && run.solvedCount < kVersesPerLevel) {
            run.active = true;
            m_run = std::move(run);
        }
    }
}

json StoryProgress::ToJson(const BibleData& bible) const {
    auto key = [&](int idx) { return VerseKey(bible.getVerseAtIndex(idx)); };
    json levels = json::object();
    for (const auto& [k, p] : m_levels) {
        json lj{{"passed", p.passed}, {"stars", StarsToJson(p.stars)}};
        json solved = json::array();
        for (int idx : p.solved) solved.push_back(key(idx));
        lj["solvedVerses"] = solved;
        if (p.retryPending) lj["retryPending"] = true;
        if (p.failedVerse >= 0) lj["failedVerse"] = key(p.failedVerse);
        levels[k] = lj;
    }
    json out{{"version", 1}, {"levels", levels}, {"unlocked", m_unlocked}};
    if (m_run.active) {
        json verses = json::array();
        for (int idx : m_run.verses) verses.push_back(key(idx));
        json guesses = json::array();
        for (const auto& g : m_run.currentGuesses) guesses.push_back(GuessToJson(g));
        out["inProgress"] = json{{"level", m_run.levelKey}, {"verses", verses},
                                 {"solved", m_run.solvedCount}, {"guessesSoFar", m_run.totalGuesses},
                                 {"hardMode", m_run.hardMode}, {"anyFirstTry", m_run.anyFirstTry},
                                 {"storyDone", m_run.storyDone},
                                 {"currentGuesses", guesses}};
    }
    return out;
}

int StoryProgress::TotalStars() const {
    int total = 0;
    for (const auto& [k, p] : m_levels) total += p.stars.Total();
    return total;
}

const LevelProgress* StoryProgress::Get(const std::string& key) const {
    auto it = m_levels.find(key);
    return it == m_levels.end() ? nullptr : &it->second;
}

bool StoryProgress::IsPassed(const std::string& key) const {
    const LevelProgress* p = Get(key);
    return p && p->passed;
}

void StoryProgress::Unlock(const std::string& key, std::vector<std::string>& added) {
    if (m_unlockedSet.insert(key).second) {
        m_unlocked.push_back(key);
        added.push_back(key);
    }
}

int StoryProgress::StarsNeeded(const StoryLevel& level) const {
    const StarFormula& f = level.isBonus ? level.requiresStars : level.minStars;
    return f.Eval(UnlockedCount());
}

bool StoryProgress::AllRegularLevelsMaxed(const StoryMap& map) const {
    bool any = false;
    for (const auto& area : map.Areas()) {
        if (area.bonusArea) continue;
        for (const auto& lvl : area.levels) {
            any = true;
            const LevelProgress* p = Get(lvl.key);
            if (!p || p->stars.Total() < lvl.MaxStars()) return false;
        }
    }
    return any;
}

std::vector<std::string> StoryProgress::UpdateUnlocks(const StoryMap& map) {
    std::vector<std::string> added;
    bool changed = true;
    while (changed) {
        changed = false;
        const size_t before = added.size();

        // Main paths: one level at a time, in order.
        for (const auto& path : map.Paths()) {
            for (size_t i = 0; i < path.size(); ++i) {
                if (IsUnlocked(path[i])) continue;
                if (i == 0) {
                    Unlock(path[i], added);
                } else if (IsPassed(path[i - 1])) {
                    const StoryLevel* lvl = map.FindLevel(path[i]);
                    if (lvl && TotalStars() >= StarsNeeded(*lvl)) Unlock(path[i], added);
                }
                break;
            }
        }

        // Bonus levels: once their area is reached and the stars are there.
        for (const auto& area : map.Areas()) {
            if (area.bonusArea || area.levels.empty() || !IsUnlocked(area.levels.front().key)) continue;
            for (const auto& b : area.bonusLevels) {
                if (!IsUnlocked(b.key) && TotalStars() >= StarsNeeded(b)) Unlock(b.key, added);
            }
        }

        // Bonus areas: everything else at full stars, then in order.
        for (const auto& area : map.Areas()) {
            if (!area.bonusArea || !AllRegularLevelsMaxed(map)) continue;
            for (size_t i = 0; i < area.levels.size(); ++i) {
                const std::string& key = area.levels[i].key;
                if (IsUnlocked(key)) continue;
                if (i == 0 || IsPassed(area.levels[i - 1].key)) Unlock(key, added);
                break;
            }
        }
        changed = added.size() != before;
    }
    return added;
}

bool StoryProgress::HardModeUnlocked(const std::string& key) const {
    const LevelProgress* p = Get(key);
    return p && p->stars.Total() >= kHardModeStars;
}

std::vector<int> StoryProgress::PlayablePool(const StoryLevel& level, const BibleData& bible, bool allowMature) {
    if (allowMature) return level.pool;
    std::vector<int> out;
    for (int idx : level.pool) {
        if (!bible.getVerseAtIndex(idx).mature) out.push_back(idx);
    }
    return out.size() >= kVersesPerLevel ? out : level.pool;
}

int StoryProgress::PerfectProgress(const StoryLevel& level, const BibleData& bible, bool allowMature) const {
    const LevelProgress* p = Get(level.key);
    if (!p) return 0;
    int n = 0;
    for (int idx : PlayablePool(level, bible, allowMature)) n += p->solved.count(idx) ? 1 : 0;
    return n;
}

void StoryProgress::StartRun(const StoryLevel& level, const BibleData& bible, bool allowMature, bool hardMode,
                             std::mt19937& rng) {
    // Walking away from a started attempt on another level counts as a retry.
    if (m_run.active && m_run.levelKey != level.key &&
        (m_run.solvedCount > 0 || !m_run.currentGuesses.empty())) {
        m_levels[m_run.levelKey].retryPending = true;
    }

    std::vector<int> pool = PlayablePool(level, bible, allowMature);
    if (level.fixedOrder) {
        // The tutorial: the same verses in the same order every time.
        std::vector<int> verses(pool.begin(), pool.begin() + std::min<size_t>(pool.size(), kVersesPerLevel));
        m_run = StoryRun{};
        m_run.active = true;
        m_run.levelKey = level.key;
        m_run.verses = verses;
        m_run.hardMode = hardMode;
        return;
    }
    const LevelProgress* p = Get(level.key);
    const int keep = (p && p->failedVerse >= 0 &&
                      std::find(pool.begin(), pool.end(), p->failedVerse) != pool.end())
        ? p->failedVerse : -1;
    if (keep >= 0) pool.erase(std::find(pool.begin(), pool.end(), keep));

    std::shuffle(pool.begin(), pool.end(), rng);
    std::vector<int> verses;
    if (keep >= 0) verses.push_back(keep);
    for (int idx : pool) {
        if ((int)verses.size() >= kVersesPerLevel) break;
        verses.push_back(idx);
    }
    std::shuffle(verses.begin(), verses.end(), rng);

    m_run = StoryRun{};
    m_run.active = true;
    m_run.levelKey = level.key;
    m_run.verses = verses;
    m_run.hardMode = hardMode;
}

bool StoryProgress::RecordVerseSolved(int guesses) {
    if (!m_run.active) return false;
    m_run.totalGuesses += guesses;
    if (guesses == 1) m_run.anyFirstTry = true;
    m_run.solvedCount++;
    m_run.currentGuesses.clear();
    m_run.storyDone = false;  // the "between" story (if any) comes next
    return m_run.solvedCount >= kVersesPerLevel;
}

void StoryProgress::RecordFail() {
    if (!m_run.active) return;
    LevelProgress& p = m_levels[m_run.levelKey];
    p.retryPending = true;
    if (m_run.solvedCount < (int)m_run.verses.size()) p.failedVerse = m_run.verses[m_run.solvedCount];
    m_run = StoryRun{};
}

PassResult StoryProgress::RecordPass(const StoryMap& map, const BibleData& bible, bool allowMature) {
    PassResult result;
    const StoryLevel* level = map.FindLevel(m_run.levelKey);
    if (!m_run.active || !level) return result;

    LevelProgress& p = m_levels[level->key];
    const int starsBefore = p.stars.Total();
    for (int idx : m_run.verses) p.solved.insert(idx);

    const std::vector<int> pool = PlayablePool(*level, bible, allowMature);
    result.perfectTotal = static_cast<int>(pool.size());
    for (int idx : pool) result.perfectSolved += p.solved.count(idx) ? 1 : 0;

    LevelStars& e = result.earned;
    e.guess = (m_run.hardMode ? level->hardGuessTiers : level->guessTiers).StarsFor(m_run.totalGuesses);
    e.hard = m_run.hardMode && level->allowHard;
    e.firstTry = m_run.anyFirstTry && level->allowFirstTry;
    e.noRetry = !p.retryPending && level->allowNoRetry;
    e.perfect = result.perfectSolved >= result.perfectTotal && level->allowPerfect;

    p.passed = true;
    p.stars = p.stars.Merge(e);
    p.retryPending = false;
    p.failedVerse = -1;
    result.best = p.stars;
    result.hardModeUnlocked = starsBefore < kHardModeStars && p.stars.Total() >= kHardModeStars;

    m_run = StoryRun{};
    result.newlyUnlocked = UpdateUnlocks(map);
    return result;
}
