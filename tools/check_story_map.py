"""Check story_map.json against bible_sections.json and preview the star unlock path.

Usage (from the repo root):  python tools/check_story_map.py [story_map.json] [bible_sections.json]

Pool references:  "book", "book 3", "book 3-5", "book 3:4", "book 3:4-9"
minStars:         a number (e.g. 64) or a formula "ax+b" / "ax-b" / "ax", where
                  x = levels unlocked in the whole game so far, compared to the total stars in the game.
"""
import json
import re
import sys
from collections import defaultdict

REF = re.compile(r"^\s*(\w+)(?:\s+(\d+)(?:-(\d+))?(?::(\d+)(?:-(\d+))?)?)?\s*$")
FORMULA = re.compile(r"^\s*(\d*)x\s*(?:([+-])\s*(\d+))?\s*$")
MAX_STARS = 7


def expand(ref, verses_by_book):
    m = REF.match(ref.lower())
    if not m:
        raise ValueError(f"can't read pool reference {ref!r}")
    book, c1, c2, v1, v2 = m.groups()
    if book not in verses_by_book:
        raise ValueError(f"unknown book {book!r} in {ref!r}")
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
        raise ValueError(f"{ref!r} matches no verses")
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
    bible_path = sys.argv[2] if len(sys.argv) > 2 else "bible_sections.json"
    story = json.load(open(map_path, encoding="utf-8"))
    verses_by_book = defaultdict(list)
    for v in json.load(open(bible_path, encoding="utf-8")):
        verses_by_book[v["book"]].append((v["chapter"], v["verse"]))

    errors = []
    area_ids = set()
    for area in story["areas"]:
        if area["id"] in area_ids:
            errors.append(f"duplicate area id {area['id']!r}")
        area_ids.add(area["id"])
        level_ids = set()
        for lvl in area["levels"]:
            key = f"{area['id']}.{lvl['id']}"
            if lvl["id"] in level_ids:
                errors.append(f"{key}: duplicate level id")
            level_ids.add(lvl["id"])
            refs = lvl["pool"] if isinstance(lvl["pool"], list) else [lvl["pool"]]
            pool = set()
            try:
                for r in refs:
                    pool.update(expand(r, verses_by_book))
                min_stars(lvl.get("minStars", 0), 0)
            except ValueError as e:
                errors.append(f"{key}: {e}")
            if len(pool) < 3:
                errors.append(f"{key}: pool has only {len(pool)} verses (needs 3+)")
            lvl["_size"] = len(pool)

    # Unlock preview: the player plays the main path in order and passes every level.
    # x = levels unlocked in the game when the check happens (open-from-start areas' first level counts).
    unlocked = 1 + sum(1 for a in story["areas"] if a.get("openFromStart"))
    passed = 0
    print(f"{'level':32} {'pool':>5} {'x':>3} {'need':>5} {'avg/level':>9}")
    for area in story["areas"]:
        if area.get("openFromStart"):
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

    print()
    if errors:
        print("ERRORS:")
        for e in errors:
            print("  -", e)
        sys.exit(1)
    print("No errors found.")


if __name__ == "__main__":
    main()
