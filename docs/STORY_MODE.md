# Story Mode — Design Doc

**Status:** Design (not yet implemented)

## 1. Summary

Story Mode turns Bibirble into a journey through the Bible. The player moves along a story map in
Bible order. Each **level** shows the player its own piece of story text and then asks them to guess
**3 verses** picked at random from that level's verse pool. Passing a level earns up to **7 stars**.
Stars unlock later areas and **prophetic bonus levels**. Progress **auto-saves**.

The map's content (areas, levels, story text, verse pools, star limits, unlock rules) lives in a
JSON file, `story_map.json`. **No area is hard-coded.** Changing the story means editing the JSON,
not the C++.

## 2. Goals / Non-goals

**Goals**
- A story the player reads as they go: every level carries its own part of the story.
- Replayable levels: the game picks a fresh random set of verses from the level's pool.
- A 7-star scoring system that rewards accuracy, Hard Mode, first-try guesses, clean runs and
  mastering a level's whole verse pool.
- Fully data-driven areas, so the author can add, remove, rename, reorder and regroup them freely.
- Automatic saving.

**Non-goals (for now)**
- An in-game map editor (the JSON file is the editor).
- Multiple save slots / profiles.
- Changing Daily or Random mode.

## 3. Structure

```
Story Map
 └─ Area (any number, in the order listed in the JSON)
     ├─ Level (main path, in order)
     │    ├─ story text (shown to the player)
     │    ├─ verse pool (3 are drawn per attempt)
     │    └─ star rules
     └─ Bonus level (optional, e.g. prophetic scriptures)
```

### 3.1 Areas are not set in stone
An **area** is just a named group of levels defined in `story_map.json`. The game makes no
assumptions about which areas exist, how many there are, or what they're called.

- The order of areas in the JSON **is** the order on the map.
- Area `id`s are what save data and unlock rules point to, so renaming an area's display
  `title` is always safe. Changing an `id` resets progress for that area (see §8.2).
- Story Mode areas are **separate** from the book-color areas in `BibleData::getBookArea()` /
  `tools/sort.py` `AREAS`. Those stay as they are for color feedback. Story areas can cut across
  books any way the author likes (e.g. "Joseph" spanning Genesis 37–50, or "Psalms of David"
  hand-picked across Psalms).

The real map is in `story_map.json`. The examples in this doc are just examples, and the real areas can change anytime.

### 3.2 Levels
Each level has:
- **Story text**: shown before the first verse (an `intro`), and optionally after the level is
  passed (an `outro`).
- **Verse pool**: the verses the level can ask about. Defined as any mix of:
  - whole books: `"ruth"`
  - chapters or chapter ranges: `"exodus 14"`, `"genesis 6-12"`
  - single verses or verse ranges: `"exodus 14:21"`, `"ezekiel 24:15-27"`
  The pool must resolve to **at least 3 distinct verses** (checked at load, see §9).
- **Star rules** (§5).

### 3.3 Bonus levels (prophetic scriptures)
Prophetic scriptures are **bonus levels**. They play exactly like normal levels but sit off the main
path. A bonus level unlocks only when **both** of these are true:
1. the player has reached a certain area (its `requiresArea`), **and**
2. the player has a certain star total (its `requiresStars`).

Bonus levels are never required to move forward on the main path.

**Locked bonus levels are hidden completely:** they don't appear on the map at all, with no tile,
no title, no hint and no unlock requirements. A bonus level shows up (🔮) only once it unlocks,
as a surprise. ✨

## 4. Level Flow

1. Player picks an unlocked level on the map, or presses that level's **Retry** button to replay
   it (§7.1). Hard Mode can be toggled here only if it's unlocked for this level (§5.3).
2. The **intro** story text is shown.
3. The game randomly draws **3 distinct verses** from the pool. Draws are **not seeded**, so
   every attempt is a fresh random pick.
4. The player guesses each verse with the normal game (same `GameRow` input, reveal stages and
   color feedback as Random/Daily).
5. **All 3 solved** → level passed → stars are awarded (§5), the **outro** is shown, and progress
   auto-saves.
6. **Any verse failed** (out of guesses) → the level fails, and the player **retries this level**
   (§6). No stars are awarded on a failed attempt.

Stars are only ever earned for **passing the whole level**, never per verse.

## 5. Stars (max 7 per level)

| Stars | Name | Earned when |
|---|---|---|
| ⭐⭐⭐ (0–3) | **Guess count** | Based on total guesses across all 3 verses. Fewer guesses = more stars. Limits set per level. |
| ⭐ | **Hard Mode** | The whole level was played with Hard Mode on. |
| ⭐ | **First try** | At least one verse was guessed exactly right on the first guess. |
| ⭐ | **No retry** | The level was passed on the first attempt (no failed attempts before it). |
| ⭐ | **Perfect** | Every verse in the level's pool has been solved, counted across all plays of the level. |

### 5.1 Guess-count stars
Each level sets the most total guesses (across the 3 verses) allowed for each star tier:

```json
"guessStars": { "3": 6, "2": 9, "1": 12 }
```
= 3 stars for ≤ 6 total guesses, 2 for ≤ 9, 1 for ≤ 12, and 0 above that (still a pass).

If a level leaves `guessStars` out, it uses the map-wide `defaults.guessStars`.

**Tiers scale in Hard Mode.** Hard Mode is tougher, so its tiers allow more guesses. A level can set
its own `hardGuessStars`. If it doesn't, each normal limit is multiplied by
`defaults.hardModeScale` (default `1.25`) and rounded up, e.g. `{6, 9, 12}` → `{8, 12, 15}`.

### 5.2 Bonus stars
- **Hard Mode** uses the existing Hard Mode constraint checks in `GameState`. Turning Hard Mode
  on/off mid-level isn't allowed: it's set when the level starts.
- **First try**: a verse whose first `GuessRecord` is all green (book + all digits).
- **No retry**: the level was passed without failing a verse during that attempt (§6).
- **Perfect**: each verse solved in a passed attempt is added to the level's saved
  `solvedVerses` set. The star is earned once that set covers **every verse in the pool**. Since
  draws are random, replaying with **Retry** (§7.1) is how players work toward it. Big pools =
  a real challenge. 🏆

Any bonus star can be turned off for a level with `"bonus": [...]` listing only the ones it allows.
By default all four are on.

### 5.3 Hard Mode unlock
Hard Mode is **locked per level** until the player has earned **4 or more stars** on that level.
After that, the Hard Mode toggle is available when starting or retrying that level. So the usual
path is: pass the level → collect 4+ stars → hit **Retry** in Hard Mode for the Hard Mode star.

### 5.4 Best score is kept
A level's saved stars are the **best** result ever, per category. Replaying a passed level can only
add stars, never remove them. For example, an earlier Hard Mode star stays even if a later run is
played in normal mode.

The **star total** used for unlocks = the sum of saved stars over all levels (bonus levels included).

## 6. Fail & Retry

When any verse in a level is failed:
- The attempt ends and the player retries **the same level**.
- The retry draws a **new random set of verses**, but **the verse the player failed on is always
  included** so they get a shot at it again. The other 2 are re-rolled from the rest of the pool.
- The **No retry** star can't be earned on the attempt that eventually passes after a fail.
  It can still be earned later by replaying the level with **Retry** (§7.1) and passing cleanly.

## 7. Unlocking

Levels unlock **one at a time, in map order**. A level unlocks when:
1. the level before it on the path is passed, **and**
2. the player's **total stars in the whole game** ≥ that level's `minStars`.

`minStars` is either a plain number (`64`) or a formula `ax+b` (`"2x"`, `"3x+1"`, `"6x-10"`), where
**x = the number of levels unlocked in the whole game so far**. That way the bar rises as the player
goes deeper. A formula below 0 counts as 0. Each level has its own `minStars`, so when a book is split
into several smaller levels, every piece uses the book's formula (and x keeps going up).

Example: with 37 levels unlocked, `"4x+3"` needs 4 × 37 + 3 = **151** total stars.

- **Start:** the first level of the first area is always unlocked.
- **Open-from-start areas:** an area with `"openFromStart": true` (e.g. the Gospels) is its own
  path. Its first level is unlocked from the start, and its levels then unlock in order as usual.
- **Next area:** the first level of an area follows on from the last level of the area before it.
- **Bonus levels:** see §3.3 (`requiresArea` + `requiresStars`).
- **Hard Mode:** per level, at 4+ stars (§5.3).

`tools/check_story_map.py` prints the star total each level needs along the main path, plus the
average stars per level that implies, so the curve can be tuned.

### 7.1 Retry button
Every **passed** level on the map has a **Retry** button 🔁 to play it again from scratch. A new
random draw, new attempt, and a chance to raise its stars (best score is kept, §5.4). This is how
players earn the Hard Mode, No retry and Perfect stars they missed the first time.

## 8. Saving

### 8.1 Auto-save
Progress is saved automatically through `PersistenceManager` (the same per-user JSON file),
under a new `story` key. It saves:
- after each verse is solved (so the in-progress level can be resumed),
- when a level is passed or failed,
- when the app closes.

There is no manual save button.

### 8.2 Save format
```json
"story": {
  "mapVersion": 1,
  "levels": {
    "exodus.red-sea": {
      "passed": true,
      "stars": { "guess": 2, "hard": true, "firstTry": true, "noRetry": false, "perfect": false },
      "solvedVerses": ["Exodus 14:21", "Exodus 14:22", "Exodus 15:2"]
    }
  },
  "inProgress": {
    "level": "exodus.passover",
    "verses": ["Exodus 12:13", "Exodus 12:23", "Exodus 12:27"],
    "solved": 1,
    "guessesSoFar": 3,
    "failedThisAttempt": false,
    "hardMode": false
  }
}
```

Saved progress is keyed by `areaId.levelId`, so:
- reordering or renaming areas/levels keeps progress,
- deleting a level drops its saved stars (they no longer count toward the total),
- adding a level just adds a new locked/unplayed level.

## 9. `story_map.json` format

```json
{
  "version": 1,
  "defaults": {
    "guessStars": { "3": 6, "2": 9, "1": 12 },
    "hardModeScale": 1.25
  },
  "areas": [
    {
      "id": "creation",
      "title": "In the Beginning",
      "intro": "Before anything was, God was...",
      "levels": [
        {
          "id": "days",
          "title": "Six Days",
          "intro": "God spoke, and light broke through the dark...",
          "outro": "And God saw that it was very good.",
          "pool": ["genesis 1", "genesis 2:1-3"],
          "minStars": 0
        }
      ]
    },
    {
      "id": "exodus",
      "title": "Out of Egypt",
      "levels": [
        {
          "id": "red-sea",
          "title": "The Red Sea",
          "intro": "Pharaoh's army thundered behind them. Moses raised his staff...",
          "pool": ["exodus 14:13-31", "exodus 15:1-3"],
          "minStars": "2x",
          "guessStars": { "3": 5, "2": 8, "1": 11 },
          "hardGuessStars": { "3": 7, "2": 10, "1": 14 },
          "bonus": ["hard", "firstTry", "noRetry", "perfect"]
        }
      ],
      "bonusLevels": [
        {
          "id": "prophet-like-moses",
          "title": "A Prophet Like Me",
          "requiresArea": "exodus",
          "requiresStars": 20,
          "intro": "Moses spoke of One who would come...",
          "pool": "deuteronomy 18:15-19"
        }
      ]
    }
  ]
}
```

Pool references are lowercase book names as in `bible_sections.json` (`1samuel`, `2kings`), in
any of these forms: `"ruth"` (whole book), `"genesis 6-12"`, `"2chronicles 36"`, `"ezekiel 24:15-27"`.
A pool can be one string or a list. The real map lives in `story_map.json` at the repo root.

### 9.1 Load-time validation
When the map loads, the game checks it and reports clear errors (area/level id + problem) for:
- duplicate area ids, or duplicate level ids within an area,
- a pool reference that doesn't exist in `bible_sections.json` (uses `BibleData::verseExists()`),
- a pool with fewer than 3 distinct verses,
- a `minStars` that isn't a number or an `ax+b` formula,
- `guessStars` / `hardGuessStars` tiers that aren't increasing (`3` ≤ `2` ≤ `1`),
- a `requiresArea` that points at an unknown area.

### 9.2 Where the file lives
Same approach as `bible_sections.json`: compiled into the program (Windows `.rc`, Linux
`.incbin`), with a `story_map.json` found on disk taking priority. That lets the author edit the
story and test it **without rebuilding**.

## 10. UI

- **Start screen:** a new **Story** button next to Daily / Random.
- **Map screen** (new `StoryMapScreen` panel, swapped in by `BibirbleWindow` like
  `StartScreen`/`SettingsScreen`):
  - areas listed in order, with locked areas greyed out and showing their unlock requirement 🔒,
  - levels in each area showing their best stars (e.g. ⭐⭐⭐⭐⭐☆☆),
  - a **Retry** 🔁 button on every passed level, plus a Hard Mode toggle once it has 4+ stars,
  - Perfect progress per level (e.g. "Pool 9 / 14"),
  - unlocked bonus levels marked with 🔮 (locked ones are not shown at all),
  - the player's star total at the top.
- **Story card:** intro/outro text with a Continue button.
- **In-level HUD:** "Verse 2 / 3".
- **Level result:** stars earned in each category, with any new bests highlighted (and "Hard
  Mode unlocked!" the first time a level reaches 4 stars 💪).

## 11. Code changes (planned)

| Piece | Change |
|---|---|
| `src/StoryMap.h/cpp` (new) | Loads + validates `story_map.json`; expands pool references into verse indices. |
| `src/StoryProgress.h/cpp` (new) | Attempts, best stars, in-progress level, star total, unlock checks. |
| `src/StoryMapScreen.h/cpp` (new) | Map UI panel, including Retry buttons and the per-level Hard Mode toggle. |
| `GameState` | Add `GameMode::Story` plus level context (level id, verse 1–3 of 3). |
| `BibirbleWindow` | Runs the 3-verse level loop, retry logic, and story cards. |
| `PersistenceManager` | Read/write the `story` key. |
| `StartScreen` | Story button. |
| `CMakeLists.txt` / `Bibirble.rc` | Embed `story_map.json`. |
| `tools/check_story_map.py` | Validates `story_map.json` and previews the star unlock curve (done). |

## 12. Decisions
- Random draws are **not seeded**. Each attempt is a fresh random pick.
- Locked bonus levels are **hidden completely**, and appear only once unlocked.
- Guess-count tiers **scale in Hard Mode** (`hardGuessStars` or `hardModeScale`).
- Hard Mode unlocks per level at **4+ stars**, and every passed level has a **Retry** button.
