// Story Mode logic tests (no wx). Built by CMake as `story_tests`, run with ctest.
// BIBIRBLE_SOURCE_DIR points at the repo so the real bible_sections.json and
// story_map.json can be checked too.
#include "BibleData.h"
#include "StoryMap.h"
#include "StoryProgress.h"
#include "WordPuzzle.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace {
int g_failures = 0;
int g_checks = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_failures; \
    std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Solves every verse of the current run, `guesses` tries each.
PassResult PassRun(StoryProgress& p, const StoryMap& map, const BibleData& bible, int guesses) {
    for (int i = 0; i < StoryProgress::kVersesPerLevel; ++i) p.RecordVerseSolved(guesses);
    return p.RecordPass(map, bible, false);
}

void TestFormulas() {
    StarFormula f;
    CHECK(StarFormula::Parse(json("4x-10"), f) && f.a == 4 && f.b == -10);
    CHECK(f.Eval(2) == 0);      // never below 0
    CHECK(f.Eval(10) == 30);
    CHECK(StarFormula::Parse(json("3x+1"), f) && f.Eval(5) == 16);
    CHECK(StarFormula::Parse(json("x"), f) && f.Eval(7) == 7);
    CHECK(StarFormula::Parse(json(64), f) && f.Eval(100) == 64);
    CHECK(StarFormula::Parse(json("80"), f) && f.Eval(3) == 80);
    CHECK(!StarFormula::Parse(json("lots"), f));

    GuessTiers t{6, 9, 12};
    CHECK(t.StarsFor(3) == 3 && t.StarsFor(6) == 3 && t.StarsFor(7) == 2);
    CHECK(t.StarsFor(12) == 1 && t.StarsFor(13) == 0);
}

void TestRealMap(const BibleData& bible, const std::string& dir) {
    StoryMap map;
    const bool ok = map.LoadFromText(ReadFile(dir + "/story_map.json"), bible);
    for (const auto& e : map.Errors()) std::printf("  map error: %s\n", e.c_str());
    CHECK(ok);
    const auto paths = map.Paths();
    CHECK(paths.size() == 2);   // Old Testament path, and the Gospels -> Acts path
    CHECK(map.FindArea("end-times") && map.FindArea("end-times")->bonusArea);
    const StoryLevel* jericho = map.FindLevel("law.jericho");
    CHECK(jericho && jericho->minStars.Eval(25) == 77);
    CHECK(jericho && jericho->pool.size() > 100);
    // Hard Mode tiers scale by hardModeScale (1.25, rounded up): 6/9/12 -> 8/12/15.
    CHECK(jericho && jericho->hardGuessTiers.tier3 == 8 && jericho->hardGuessTiers.tier1 == 15);

    StoryProgress p;
    p.UpdateUnlocks(map);
    CHECK(p.IsUnlocked("creation.eden") && p.IsUnlocked("messiah.matthew-birth"));
    CHECK(!p.IsUnlocked("creation.fall") && !p.IsUnlocked("end-times.last-days"));
}

const char* kTestMap = R"({
  "defaults": { "guessStars": { "3": 3, "2": 6, "1": 9 } },
  "areas": [
    { "id": "a", "levels": [
        { "id": "one", "pool": "ruth 1:1-3", "minStars": 0 },
        { "id": "two", "pool": ["ruth 1:4-6", "ruth 2:1"], "minStars": "2x" } ],
      "bonusLevels": [ { "id": "b", "pool": "joel 1:1-3", "requiresStars": 5 } ] },
    { "id": "n", "openFromStart": true, "levels": [ { "id": "one", "pool": "mark 1:1-3" } ] },
    { "id": "end", "bonusArea": true, "unlock": "perfect", "levels": [
        { "id": "e1", "pool": "revelation 1:1-3" }, { "id": "e2", "pool": "revelation 2:1-3" } ] }
  ]
})";

void TestUnlocksAndStars(const BibleData& bible) {
    StoryMap map;
    CHECK(map.LoadFromText(kTestMap, bible));
    std::mt19937 rng(1234);
    StoryProgress p;
    p.UpdateUnlocks(map);
    CHECK(p.UnlockedCount() == 2);  // a.one and n.one
    CHECK(!p.IsUnlocked("a.b"));    // bonus levels stay hidden until unlocked

    // Pass a.one with 1 guess per verse: 3 guess stars + first try + no retry
    // + perfect (the 3-verse pool is all solved) = 6 of 7.
    const StoryLevel* one = map.FindLevel("a.one");
    p.StartRun(*one, bible, false, false, rng);
    std::set<int> drawn(p.Run().verses.begin(), p.Run().verses.end());
    CHECK(drawn.size() == 3);
    PassResult r = PassRun(p, map, bible, 1);
    CHECK(r.earned.guess == 3 && r.earned.firstTry && r.earned.noRetry && r.earned.perfect && !r.earned.hard);
    CHECK(r.best.Total() == 6 && p.TotalStars() == 6);
    CHECK(r.hardModeUnlocked && p.HardModeUnlocked("a.one"));
    // a.two needs 2x = 4 stars at x = 2; the bonus level needs 5.
    CHECK(p.IsUnlocked("a.two") && p.IsUnlocked("a.b"));

    // Fail a.two: the failed verse is kept for the retry, and No retry is lost.
    const StoryLevel* two = map.FindLevel("a.two");
    p.StartRun(*two, bible, false, false, rng);
    const int failedVerse = p.Run().verses[0];
    p.RecordFail();
    CHECK(!p.Run().active && p.Get("a.two")->retryPending && p.Get("a.two")->failedVerse == failedVerse);
    p.StartRun(*two, bible, false, false, rng);
    const auto& v = p.Run().verses;
    CHECK(std::find(v.begin(), v.end(), failedVerse) != v.end());
    r = PassRun(p, map, bible, 3);  // 9 guesses: 1 guess star with tiers 3/6/9
    CHECK(r.earned.guess == 1 && !r.earned.noRetry && !r.earned.firstTry);
    CHECK(!r.earned.perfect && r.perfectTotal == 4 && r.perfectSolved == 3);
    CHECK(!p.Get("a.two")->retryPending);

    // Replaying keeps the best stars per category.
    p.StartRun(*two, bible, false, true, rng);
    r = PassRun(p, map, bible, 6);  // hard tiers scale 3/6/9 -> 4/8/12: 18 guesses = 0
    CHECK(r.earned.hard && r.earned.noRetry && r.earned.guess == 0);
    CHECK(r.best.guess == 1 && r.best.hard && r.best.noRetry);

    // Abandoning a started attempt counts as a retry.
    p.StartRun(*two, bible, false, false, rng);
    p.RecordVerseSolved(2);
    p.StartRun(*one, bible, false, false, rng);
    CHECK(p.Get("a.two")->retryPending);
    p.ClearRun();

    // The bonus area only opens when every regular level has all its stars.
    CHECK(!p.IsUnlocked("end.e1"));
    LevelProgress& full = p.At("a.one");
    full.stars = LevelStars{3, true, true, true, true};
    p.At("a.two").stars = LevelStars{3, true, true, true, true};
    p.UpdateUnlocks(map);
    CHECK(!p.IsUnlocked("end.e1"));  // n.one isn't passed yet
    p.At("n.one").stars = LevelStars{3, true, true, true, true};
    p.At("n.one").passed = true;
    p.UpdateUnlocks(map);
    CHECK(p.IsUnlocked("end.e1") && !p.IsUnlocked("end.e2"));

    // Save and load round trip, including an attempt in progress.
    p.StartRun(*two, bible, false, true, rng);
    p.RecordVerseSolved(4);
    GuessRecord g;
    g.bookGuess = "ruth";
    g.bookColor = GuessColor::Green;
    g.digitGuesses = {"0", "1", "0", "5"};
    g.digitColors = {GuessColor::Green, GuessColor::Green, GuessColor::Gray, GuessColor::Yellow};
    p.Run().currentGuesses.push_back(g);
    StoryProgress loaded;
    loaded.FromJson(p.ToJson(bible), bible);
    CHECK(loaded.TotalStars() == p.TotalStars());
    CHECK(loaded.UnlockedCount() == p.UnlockedCount() && loaded.IsUnlocked("end.e1"));
    CHECK(loaded.Get("a.two")->solved == p.Get("a.two")->solved);
    CHECK(loaded.Run().active && loaded.Run().verses == p.Run().verses);
    CHECK(loaded.Run().solvedCount == 1 && loaded.Run().totalGuesses == 4 && loaded.Run().hardMode);
    CHECK(loaded.Run().currentGuesses.size() == 1 &&
          loaded.Run().currentGuesses[0].digitColors[3] == GuessColor::Yellow);
}

void TestWordPuzzles(const BibleData& bible, const std::string& dir) {
    using G = GuessColor;
    // Wordle colouring, including repeated letters.
    CHECK((ScoreWord("ark", "ark") == std::vector<G>{G::Green, G::Green, G::Green}));
    CHECK((ScoreWord("kar", "ark") == std::vector<G>{G::Yellow, G::Yellow, G::Yellow}));
    CHECK((ScoreWord("llama", "lambs") == std::vector<G>{G::Green, G::Gray, G::Yellow, G::Yellow, G::Gray}));
    CHECK((ScoreWord("eerie", "there") == std::vector<G>{G::Yellow, G::Gray, G::Yellow, G::Gray, G::Green}));

    StoryText t;
    std::string err;
    CHECK(StoryText::Parse("God told Noah to build an {{Ark|A huge boat}} of {{wood}}.", t, err));
    CHECK(t.blanks.size() == 2 && t.blanks[0].answer == "ark" && t.blanks[0].hint == "A huge boat");
    CHECK(t.blanks[1].hint.empty() && t.parts.size() == 3);
    CHECK(t.Render(0) == "God told Noah to build an [ _ _ _ ] of _ _ _ _.");
    CHECK(t.Render(1) == "God told Noah to build an ARK of [ _ _ _ _ ].");
    CHECK(t.Render(2) == "God told Noah to build an ARK of WOOD.");
    CHECK(StoryText::Parse("No puzzles here.", t, err) && t.blanks.empty() && !t.Empty());
    CHECK(StoryText::Parse("", t, err) && t.Empty());
    CHECK(!StoryText::Parse("Oops {{ark", t, err));
    CHECK(!StoryText::Parse("Two words {{burning bush}}", t, err));
    CHECK(!StoryText::Parse("Empty {{|hint}}", t, err));
    CHECK(DefaultHint("manna") == "It starts with M and has 5 letters.");

    // Hard mode: greens stay put, yellows (and greens) must be reused.
    std::vector<WordGuess> hist = {{"night", ScoreWord("night", "light")}};   // N gray, IGHT green
    CHECK(WordHardModeViolation(hist, "sight").empty());
    CHECK(WordHardModeViolation(hist, "light").empty());
    CHECK(WordHardModeViolation(hist, "lithe") == "Hard mode: keep the green G in spot 3.");
    CHECK(WordHardModeViolation(hist, "eight").empty());  // keeps all the greens
    CHECK(WordHardModeViolation(hist, "tight").empty());
    CHECK(!WordHardModeViolation(hist, "bough").empty());
    std::vector<WordGuess> hist2 = {{"steal", ScoreWord("steal", "light")}};  // T and L yellow
    CHECK(WordHardModeViolation(hist2, "light").empty());
    CHECK(WordHardModeViolation(hist2, "night") == "Hard mode: your guess must use the L you found.");
    CHECK(WordHardModeViolation({}, "anything").empty());

    // The real word list: dictionary words yes, capitalised names no.
    WordList words;
    words.Load(ReadFile(dir + "/data/words.txt"));
    CHECK(words.Size() > 50000);
    CHECK(words.Contains("ark") && words.Contains("manna") && words.Contains("light"));
    CHECK(!words.Contains("noah"));
    words.Load("Noah\nmoses\n");  // capitalised lines are names, so skipped
    CHECK(!words.Contains("noah") && words.Contains("moses"));

    // Blanks in the real map parse, and map answers can be added as guesses.
    StoryMap map;
    CHECK(map.LoadFromText(ReadFile(dir + "/story_map.json"), bible));
    const StoryLevel* eden = map.FindLevel("creation.eden");
    CHECK(eden && eden->intro.blanks.size() == 1 && eden->between.size() == 2 && !eden->outro.blanks.empty());
    CHECK(eden && eden->StoryBefore(0) == &eden->intro && eden->StoryBefore(2) == &eden->between[1]);
    CHECK(eden && eden->StoryBefore(3) == nullptr);
    const auto answers = map.AllAnswers();
    CHECK(std::find(answers.begin(), answers.end(), "abram") != answers.end());

    // Bad blanks and "between" lists are reported when the map loads.
    StoryMap bad;
    CHECK(!bad.LoadFromText(R"({"areas":[{"id":"a","levels":[
        {"id":"x","pool":"ruth","intro":"An {{ark"},
        {"id":"y","pool":"ruth","between":["a","b","c"]}]}]})", bible));
    CHECK(bad.Errors().size() == 2);
}

void TestStoryDone(const BibleData& bible) {
    StoryMap map;
    CHECK(map.LoadFromText(kTestMap, bible));
    std::mt19937 rng(7);
    StoryProgress p;
    p.UpdateUnlocks(map);
    p.StartRun(*map.FindLevel("a.one"), bible, false, false, rng);
    CHECK(!p.Run().storyDone);   // the intro comes first
    p.Run().storyDone = true;
    p.RecordVerseSolved(2);
    CHECK(!p.Run().storyDone);   // then the "between" story before verse 2
    StoryProgress loaded;
    loaded.FromJson(p.ToJson(bible), bible);
    CHECK(loaded.Run().active && !loaded.Run().storyDone);
}

void TestMapErrors(const BibleData& bible) {
    StoryMap map;
    CHECK(!map.LoadFromText(R"({"areas":[{"id":"a","levels":[
        {"id":"x","pool":"notabook 1"},
        {"id":"y","pool":"ruth 1:1-2"},
        {"id":"y","pool":"ruth","minStars":"lots"}]}]})", bible));
    const auto& e = map.Errors();
    auto has = [&](const std::string& s) {
        return std::any_of(e.begin(), e.end(), [&](const std::string& m) { return m.find(s) != std::string::npos; });
    };
    CHECK(has("unknown book"));
    CHECK(has("only 2 verses"));
    CHECK(has("duplicate level id"));
    CHECK(has("can't read minStars"));
}
}  // namespace

int main() {
    const std::string dir = BIBIRBLE_SOURCE_DIR;
    BibleData bible;
    if (!bible.loadData(dir + "/bible_sections.json")) {
        std::printf("couldn't load bible_sections.json\n");
        return 1;
    }
    TestFormulas();
    TestRealMap(bible, dir);
    TestUnlocksAndStars(bible);
    TestMapErrors(bible);
    TestWordPuzzles(bible, dir);
    TestStoryDone(bible);
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
