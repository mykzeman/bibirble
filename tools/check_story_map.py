"""Check story_map.json against bible_sections.json and preview the star unlock path.

Usage (from the repo root):  python tools/check_story_map.py [story_map.json] [bible_sections.json ...]

With no Bible files given it checks every version (bible_sections*.json), World English
Bible first: every pool reference must exist there; another version may lack one
(it is skipped there) as long as each pool keeps 3+ verses. Pools use
World English Bible numbering; other versions match verses by their "ref" field
(see tools/sort.py), so "psalms 23" is Psalm 22 in the Douay-Rheims.

Pool references:  "book", "book 3", "book 3-5", "book 3:4", "book 3:4-9"
Bonus levels (an area's "bonusLevels") use "requiresStars" the same way, and stay hidden until unlocked.
A bonus area ("bonusArea": true, "unlock": "perfect") opens when every non-bonus level has all 7 stars.
Story text ("intro", "outro", and "between": [after verse 1, after verse 2]) can hold
fill-in-the-blank puzzles: {{answer}} or {{answer|hint shown after 6 wrong guesses}}.
minStars:         a number (e.g. 64) or a formula "ax+b" / "ax-b" / "ax", where
                  x = levels unlocked in the whole game so far, compared to the total stars in the game.
"""
import glob
import json
import re
import sys
from collections import defaultdict

REF = re.compile(r"^\s*(\w+)(?:\s+(\d+)(?:-(\d+))?(?::(\d+)(?:-(\d+))?)?)?\s*$")
FORMULA = re.compile(r"^\s*(\d*)x\s*(?:([+-])\s*(\d+))?\s*$")
MAX_STARS = 7
BLANK = re.compile(r"\{\{(.*?)\}\}")


def check_story(text, where):
    """Errors in one story text's {{answer|hint}} blanks (see src/WordPuzzle.cpp)."""
    if text is None:
        return []
    if not isinstance(text, str):
        return [f"{where} must be text"]
    errors = []
    for inside in BLANK.findall(text):
        answer = inside.split("|", 1)[0].strip()
        if not answer:
            errors.append(f"{where}: a blank has no answer")
        elif not answer.isalpha() or not answer.isascii():
            errors.append(f"{where}: blank answer {answer!r} must be one word of letters only")
    if "{{" in BLANK.sub("", text):
        errors.append(f"{where}: a \"{{{{\" blank is never closed with \"}}}}\"")
    return errors


class NoMatch(ValueError):
    """A pool reference the Bible version doesn't have (the game skips it)."""


def expand(ref, verses_by_book):
    m = REF.match(ref.lower())
    if not m:
        raise ValueError(f"can't read pool reference {ref!r}")
    book, c1, c2, v1, v2 = m.groups()
    if book not in verses_by_book:
        raise NoMatch(f"unknown book {book!r} in {ref!r}")
    out = []
    for (ch, vs) in verses_by_book[book]:
        if c1 is None:
            ok = True
        elif v1 is not None:
            ok = ch == int(c1) and int(v1) <= vs <= int(v2 or v1)
        else:
            ok = int(c1) <= ch <= int(c2 or c1)
        if ok:
            out.append((book, ch, vs))
    if not out:
        raise NoMatch(f"{ref!r} matches no verses")
    return out


def min_stars(value, x):
    if isinstance(value, int):
        return value
    m = FORMULA.match(str(value))
    if not m:
        raise ValueError(f"can't read minStars {value!r}")
    a = int(m.group(1) or 1)
    b = int(m.group(3) or 0) * (-1 if m.group(2) == "-" else 1)
    return max(0, a * x + b)


def main():
    map_path = sys.argv[1] if len(sys.argv) > 1 else "story_map.json"
    bible_paths = sys.argv[2:] or sorted(glob.glob("bible_sections*.json"))  # bible_sections.json sorts first
    failed = False
    for i, bible_path in enumerate(bible_paths):
        print(f"=== {bible_path}")
        failed |= not check(map_path, bible_path, preview=(i == 0))
    sys.exit(1 if failed else 0)


def check(map_path, bible_path, preview):
    story = json.load(open(map_path, encoding="utf-8"))
    verses_by_book = defaultdict(list)
    for v in json.load(open(bible_path, encoding="utf-8")):
        ref = v.get("ref", [v["chapter"], v["verse"]])
        if ref is not None:
            verses_by_book[v["book"]].append(tuple(ref))

    errors = []
    area_ids = set()
    for area in story["areas"]:
        if area["id"] in area_ids:
            errors.append(f"duplicate area id {area['id']!r}")
        area_ids.add(area["id"])
        level_ids = set()
        for lvl in area["levels"] + area.get("bonusLevels", []):
            key = f"{area['id']}.{lvl['id']}"
            if lvl["id"] in level_ids:
                errors.append(f"{key}: duplicate level id")
            level_ids.add(lvl["id"])
            refs = lvl["pool"] if isinstance(lvl["pool"], list) else [lvl["pool"]]
            pool = set()
            try:
                for r in refs:
                    try:
                        pool.update(expand(r, verses_by_book))
                    except NoMatch as e:
                        # Other versions may lack a passage (the Douay-Rheims
                        # has no Malachi 4): the game skips it there. The
                        # World English Bible (checked first) must have it all.
                        if preview:
                            raise
                        print(f"  note: {key}: {e}, skipped in this version")
                min_stars(lvl.get("minStars", 0), 0)
                min_stars(lvl.get("requiresStars", 0), 0)
            except ValueError as e:
                errors.append(f"{key}: {e}")
            for where in ("intro", "outro"):
                errors += [f"{key}: {e}" for e in check_story(lvl.get(where), where)]
            between = lvl.get("between", [])
            if not isinstance(between, list) or len(between) > 2:
                errors.append(f"{key}: between must be a list of up to 2 texts (after verse 1, after verse 2)")
            else:
                for b in between:
                    errors += [f"{key}: {e}" for e in check_story(b, "between")]
            if len(pool) < 3:
                errors.append(f"{key}: pool has only {len(pool)} verses (needs 3+)")
            lvl["_size"] = len(pool)
        if area.get("bonusArea") and area.get("unlock") != "perfect":
            errors.append(f"{area['id']}: bonus area needs \"unlock\": \"perfect\"")

    if not preview:
        return report(errors)

    # Unlock preview: the player plays the main path in order and passes every level.
    # x = levels unlocked in the game when the check happens (open-from-start areas' first level counts).
    unlocked = 1 + sum(1 for a in story["areas"] if a.get("openFromStart"))
    passed = 0
    print(f"{'level':32} {'pool':>5} {'x':>3} {'need':>5} {'avg/level':>9}")
    for area in story["areas"]:
        if area.get("openFromStart") or area.get("bonusArea"):
            continue
        for i, lvl in enumerate(area["levels"]):
            key = f"{area['id']}.{lvl['id']}"
            if area is story["areas"][0] and i == 0:
                passed += 1
                print(f"{key:32} {lvl['_size']:>5} {'-':>3} {'start':>5}")
                continue
            need = min_stars(lvl.get("minStars", 0), unlocked)
            avg = need / passed if passed else 0
            flag = "  <-- impossible" if avg > MAX_STARS else ("  <-- needs Perfect stars" if avg > 6 else ("  <-- very hard" if avg > 5 else ""))
            print(f"{key:32} {lvl['_size']:>5} {unlocked:>3} {need:>5} {avg:>9.2f}{flag}")
            unlocked += 1
            passed += 1
        for b in area.get("bonusLevels", []):
            need = min_stars(b.get("requiresStars", 0), unlocked)
            print(f"  (bonus) {b['id']:23} {b['_size']:>5} {unlocked:>3} {need:>5} {need / passed:>9.2f}")

    # The perfect-unlock bonus area needs every non-bonus level at 7 stars, including Perfect
    # (every verse in every pool solved). Show how big that grind is.
    main_verses = sum(l["_size"] for a in story["areas"] if not a.get("bonusArea") for l in a["levels"])
    print(f"\nPerfect-unlock area needs every verse of every non-bonus level solved: "
          f"{main_verses} verses, at least {-(-main_verses // 3)} level plays.")

    return report(errors)


def report(errors):
    print()
    if errors:
        print("ERRORS:")
        for e in errors:
            print("  -", e)
        return False
    print("No errors found.")
    return True


if __name__ == "__main__":
    main()
