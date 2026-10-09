"""Automatically choose the verse pool for "Useful verses only" mode.

A verse counts as useful when it has few commas and few numbers: lots of
commas usually means a list (genealogies, names, places) and lots of numbers
means a census, measurements, or ages. Useful verses are then ranked by book
and chapter, and the pool keeps the hand-picked verses in
tools/useful_must_include.txt plus the best-ranked useful verses until it
reaches --count. This only affects Useful mode; Daily and Random games
without it still use every verse.

Writes src/UsefulVerses.h and, when the web repo is next to this one,
../Bibirble-web/scripts/useful.js, so both games share the same pool.

Run from the repo root:
    python tools/pick_useful.py               # write both files
    python tools/pick_useful.py --dry-run     # just print the picks
    python tools/pick_useful.py --count 500 --explain "john 3:16"
"""
import argparse
import json
import re
from pathlib import Path

# --- Book: how often a book's verses are quoted, memorized, or preached. ---
BOOK_WEIGHTS = {
    "psalms": 3.0, "proverbs": 3.0, "john": 3.0, "romans": 3.0,
    "matthew": 2.5, "philippians": 2.5, "ephesians": 2.5, "1john": 2.5,
    "isaiah": 2.0, "galatians": 2.0, "hebrews": 2.0, "james": 2.0,
    "1corinthians": 2.0, "2corinthians": 2.0, "colossians": 2.0,
    "1peter": 2.0, "luke": 2.0, "mark": 1.5, "2timothy": 1.5,
    "1thessalonians": 1.5, "genesis": 1.5, "deuteronomy": 1.5,
    "joshua": 1.5, "jeremiah": 1.5, "lamentations": 1.5, "micah": 1.5,
    "ecclesiastes": 1.5, "revelation": 1.5, "acts": 1.5, "titus": 1.0,
    "1timothy": 1.0, "exodus": 1.0, "job": 1.0, "habakkuk": 1.0,
    "zephaniah": 1.0, "ruth": 1.0, "daniel": 1.0, "2peter": 1.0,
    "jude": 1.0, "nahum": 0.5, "joel": 0.5, "hosea": 0.5, "malachi": 0.5,
    "1samuel": 0.5, "2samuel": 0.5, "esther": 0.5, "jonah": 0.5,
    "songofsolomon": 0.0, "2thessalonians": 0.5, "philemon": 0.0,
    "2john": 0.0, "3john": 0.0, "1kings": 0.0, "2kings": 0.0,
    "judges": 0.0, "amos": 0.0, "obadiah": 0.0, "haggai": 0.0,
    "zechariah": 0.0, "ezekiel": -0.5, "ezra": -1.0, "nehemiah": -1.0,
    "2chronicles": -1.0, "1chronicles": -1.5, "leviticus": -1.5,
    "numbers": -1.5,
}

# --- Chapter: chapters people know by heart. ---
FAMOUS_CHAPTERS = {
    ("genesis", 1), ("genesis", 3), ("exodus", 20), ("deuteronomy", 6),
    ("joshua", 1), ("psalms", 1), ("psalms", 19), ("psalms", 23),
    ("psalms", 27), ("psalms", 34), ("psalms", 37), ("psalms", 46),
    ("psalms", 51), ("psalms", 91), ("psalms", 96), ("proverbs", 3),
    ("ecclesiastes", 3), ("isaiah", 9), ("isaiah", 40), ("isaiah", 53),
    ("isaiah", 55), ("jeremiah", 29), ("lamentations", 3), ("matthew", 5),
    ("matthew", 6), ("matthew", 7), ("matthew", 28), ("luke", 2),
    ("luke", 15), ("john", 1), ("john", 3), ("john", 10), ("john", 11),
    ("john", 14), ("john", 15), ("romans", 3), ("romans", 5),
    ("romans", 6), ("romans", 8), ("romans", 12), ("1corinthians", 13),
    ("1corinthians", 15), ("2corinthians", 5), ("galatians", 5),
    ("ephesians", 2), ("ephesians", 6), ("philippians", 2),
    ("philippians", 4), ("colossians", 3), ("hebrews", 11),
    ("hebrews", 12), ("james", 1), ("1peter", 5), ("1john", 1),
    ("1john", 4), ("revelation", 21), ("revelation", 22),
}
FAMOUS_CHAPTER_BONUS = 2.0

# --- Content: commas and numbers decide whether a verse is useful. ---
# Numbers are mostly spelled out in the World English Bible ("two hundred
# thirty"), so number words count as well as digits.
NUMBER_WORDS = {
    "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
    "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
    "seventeen", "eighteen", "nineteen", "twenty", "thirty", "forty",
    "fifty", "sixty", "seventy", "eighty", "ninety", "hundred", "thousand",
    "thousands", "hundreds", "first", "second", "third", "fourth", "fifth",
    "sixth", "seventh", "eighth", "ninth", "tenth", "twelfth",
}
MAX_COMMAS = 4               # more than this reads like a list
MAX_COMMAS_PER_10_WORDS = 1.5
MAX_NUMBERS = 1              # more than this reads like a census or measurement
COMMA_PENALTY = 0.3          # ranking tiebreak among useful verses
NUMBER_PENALTY = 1.0
MAX_PER_CHAPTER = 3         # keep the pool varied
MAX_BOOK_SHARE = 0.08       # no book over 8% of the pool


def load_refs(path):
    refs = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        book, cv = line.split()
        chapter, verse = cv.split(":")
        refs.append((book, int(chapter), int(verse)))
    return refs


def normalize(text):
    return re.sub(r"[^a-z ]", "", text.lower())


def count_commas(text):
    return text.count(",")


def count_numbers(text):
    words = re.findall(r"[a-z]+|\d+", text.lower())
    return sum(1 for w in words if w.isdigit() or w in NUMBER_WORDS)


def is_useful(v):
    commas = count_commas(v["text"])
    numbers = count_numbers(v["text"])
    words = len(v["text"].split())
    return (commas <= MAX_COMMAS
            and commas * 10 / words <= MAX_COMMAS_PER_10_WORDS
            and numbers <= MAX_NUMBERS)


def score_verse(v, explain=False):
    """Ranks useful verses: book and chapter first, then fewer commas/numbers."""
    commas = count_commas(v["text"])
    numbers = count_numbers(v["text"])
    parts = [("book", BOOK_WEIGHTS.get(v["book"], 0.0))]
    if (v["book"], v["chapter"]) in FAMOUS_CHAPTERS:
        parts.append(("famous chapter", FAMOUS_CHAPTER_BONUS))
    parts.append((f"{commas} commas", -commas * COMMA_PENALTY))
    parts.append((f"{numbers} numbers", -numbers * NUMBER_PENALTY))
    total = sum(p for _, p in parts)
    if explain:
        return total, parts
    return total


def pick(verses, must_include, count):
    by_ref = {(v["book"], v["chapter"], v["verse"]): v for v in verses}
    chosen = []
    missing = []
    for ref in must_include:
        if ref in by_ref:
            if ref not in chosen:
                chosen.append(ref)
        else:
            missing.append(ref)

    per_chapter = {}
    per_book = {}
    for b, c, _ in chosen:
        per_chapter[(b, c)] = per_chapter.get((b, c), 0) + 1
        per_book[b] = per_book.get(b, 0) + 1
    book_cap = max(1, int(count * MAX_BOOK_SHARE))

    # The same wording in several places (e.g. a refrain) is only taken once.
    seen_texts = {normalize(by_ref[r]["text"]) for r in chosen}

    useful = [v for v in verses if is_useful(v)]
    ranked = sorted(useful, key=lambda v: (-score_verse(v), v["book"], v["chapter"], v["verse"]))
    for v in ranked:
        if len(chosen) >= count:
            break
        ref = (v["book"], v["chapter"], v["verse"])
        if ref in chosen:
            continue
        if per_chapter.get(ref[:2], 0) >= MAX_PER_CHAPTER or per_book.get(ref[0], 0) >= book_cap:
            continue
        if normalize(v["text"]) in seen_texts:
            continue
        seen_texts.add(normalize(v["text"]))
        chosen.append(ref)
        per_chapter[ref[:2]] = per_chapter.get(ref[:2], 0) + 1
        per_book[ref[0]] = per_book.get(ref[0], 0) + 1

    # Dataset order, so both games map a seed to the same verse.
    order = {(v["book"], v["chapter"], v["verse"]): i for i, v in enumerate(verses)}
    chosen.sort(key=order.get)
    return chosen, missing


HEADER_COMMENT = (
    'Curated pool of well-known, memorable verses used by "Useful verses only"\n'
    "mode, so games skip genealogies, census counts, and similar passages.\n"
    "Generated by bibirble's tools/pick_useful.py; edit tools/useful_must_include.txt\n"
    "or the scoring in that script and re-run it instead of editing by hand.\n"
)


def write_header(path, refs):
    comment = "".join(f"// {line}\n" for line in HEADER_COMMENT.splitlines())
    body = "".join(f'    {{"{b}", {c}, {v}}},\n' for b, c, v in refs)
    Path(path).write_text(
        "#pragma once\n\n" + comment +
        "\nstruct UsefulVerseRef {\n    const char* book;\n    int chapter;\n    int verse;\n};\n\n"
        "inline constexpr UsefulVerseRef kUsefulVerses[] = {\n" + body + "};\n",
        encoding="utf-8")


def write_js(path, refs):
    comment = "".join(f"// {line}\n" for line in HEADER_COMMENT.splitlines())
    body = "".join(f'    ["{b}", {c}, {v}],\n' for b, c, v in refs)
    Path(path).write_text(comment + "export const USEFUL_VERSES = [\n" + body + "];\n",
                          encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--data", default="bible_sections.json")
    parser.add_argument("--must-include", default="tools/useful_must_include.txt")
    parser.add_argument("--count", type=int, default=365, help="pool size (default: 365)")
    parser.add_argument("--header", default="src/UsefulVerses.h")
    parser.add_argument("--js", default="../Bibirble-web/scripts/useful.js",
                        help="web output; skipped if its folder doesn't exist")
    parser.add_argument("--dry-run", action="store_true", help="print picks, write nothing")
    parser.add_argument("--explain", metavar="REF", help='show the score of one verse, e.g. "john 3:16"')
    args = parser.parse_args()

    verses = json.loads(Path(args.data).read_text(encoding="utf-8"))

    if args.explain:
        book, cv = args.explain.split()
        chapter, verse = map(int, cv.split(":"))
        v = next((v for v in verses if (v["book"], v["chapter"], v["verse"]) == (book, chapter, verse)), None)
        if not v:
            raise SystemExit(f"{args.explain} is not in {args.data}")
        total, parts = score_verse(v, explain=True)
        print(v["text"])
        print(f"  useful: {'yes' if is_useful(v) else 'no'}")
        for name, pts in parts:
            print(f"  {name:15} {pts:+.2f}")
        print(f"  {'total':15} {total:+.2f}")
        return

    must_include = load_refs(args.must_include)
    chosen, missing = pick(verses, must_include, args.count)
    for ref in missing:
        print(f"warning: {ref[0]} {ref[1]}:{ref[2]} is not in the dataset, skipped")

    must = set(must_include)
    by_ref = {(v["book"], v["chapter"], v["verse"]): v for v in verses}
    auto = [r for r in chosen if r not in must]
    print(f"{len(chosen)} verses: {len(chosen) - len(auto)} hand-picked + {len(auto)} auto-picked")
    if args.dry_run:
        for r in auto:
            print(f"  {score_verse(by_ref[r]):5.1f}  {r[0]} {r[1]}:{r[2]}  {by_ref[r]['text'][:80]}")
        return

    write_header(args.header, chosen)
    print(f"wrote {args.header}")
    if Path(args.js).parent.is_dir():
        write_js(args.js, chosen)
        print(f"wrote {args.js}")
    else:
        print(f"skipped {args.js} (folder not found)")


if __name__ == "__main__":
    main()
