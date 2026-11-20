"""Builds each Bible version's verse file from the per-book files in tools/bibles/<version>/.

Writes bible_sections.json (World English Bible, the default) and
bible_sections_<version>.json for the others, and copies them into
../Bibirble-web/scripts/ when the web repo is next to this one.

Run from the repo root:  python tools/sort.py

Each verse is {testament, area, book, chapter, verse, text}, plus:
  "mature": true  R18 content, hidden unless R18 mode is on.
  "ref": [c, v]   the verse's place in the World English Bible's numbering when
                  this version numbers it differently (the Douay-Rheims and
                  CPDV number most Psalms one lower). "ref": null means the
                  verse has no World English Bible match. Story Mode, Useful
                  verses and Christmas verses look verses up by this standard
                  reference, so they find the same passage in every version.
                  Players still see and guess the version's own numbers.
"""
import json
import re
import shutil
from pathlib import Path

from content_filters import is_mature_text

BIBLES = Path("tools/bibles")
WEB_SCRIPTS = Path("../Bibirble-web/scripts")

# Version id -> output file. Keep in sync with Bibirble-web's scripts/versions.js
# and bibirble's src/BibleVersions.h.
VERSIONS = {
    "web": "bible_sections.json",
    "kjv": "bible_sections_kjv.json",
    "dr": "bible_sections_dr.json",
    "cpdv": "bible_sections_cpdv.json",
}
# Versions that number the Psalms (and Daniel 3) like the Latin Vulgate.
VULGATE_NUMBERING = {"dr", "cpdv"}

OLD_TESTAMENT = ["genesis", "exodus", "leviticus", "numbers", "deuteronomy", "joshua", "judges", "ruth",
                 "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles", "ezra", "nehemiah",
                 "tobit", "judith", "esther", "job", "psalms", "proverbs", "ecclesiastes", "songofsolomon",
                 "wisdom", "sirach", "isaiah", "jeremiah", "lamentations", "baruch", "ezekiel", "daniel",
                 "hosea", "joel", "amos", "obadiah", "jonah", "micah", "nahum", "habakkuk", "zephaniah",
                 "haggai", "zechariah", "malachi", "1maccabees", "2maccabees"]
# Must match getBookArea() in src/BibleData.cpp and AREAS in Bibirble-web's scripts/books.js.
# The deuterocanonical books (Douay-Rheims and CPDV) join the areas they fit.
AREAS = {
    "Torah": OLD_TESTAMENT[0:5],
    "Historical": ["joshua", "judges", "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles",
                   "nehemiah", "1maccabees", "2maccabees"],
    "Poems": ["psalms", "proverbs", "ecclesiastes", "songofsolomon", "lamentations", "wisdom", "sirach"],
    "Small stories": ["job", "esther", "jonah", "ruth", "ezra", "tobit", "judith"],
    "Prophets Major": ["isaiah", "jeremiah", "ezekiel", "daniel", "baruch"],
    "Prophets Minor": ["hosea", "joel", "amos", "obadiah", "micah", "nahum", "habakkuk", "zephaniah", "haggai",
                       "zechariah", "malachi"],
    "Gospel": ["matthew", "mark", "luke", "john"],
    "Acts from Hebrews": ["acts", "hebrews"],
    "Pauls letters": ["romans", "1corinthians", "2corinthians", "galatians", "ephesians", "philippians",
                      "colossians", "1thessalonians", "2thessalonians", "1timothy", "2timothy", "titus",
                      "philemon"],
    "Peter letters": ["1peter", "2peter"],
    "James and Jude": ["james", "jude"],
    "John Letters and Visions": ["1john", "2john", "3john", "revelation"]
}
# The game's guess row has two chapter digits and two verse digits, so a
# reference past 99 (Psalms 100-150, Psalm 119:100-176) can't be entered.
MAX_REFERENCE_NUMBER = 99
MIN_WORDS = 7
# The World English Bible's files: prose arrives as "paragraph text"; poetry
# (Psalms, Proverbs, songs inside other books) as "line text", one element
# per line of the verse.
VERSE_TEXT_TYPES = ("paragraph text", "line text")


def read_book(path):
    """{(chapter, verse): text} for one book file, in either format."""
    data = json.loads(path.read_text(encoding="utf-8"))
    verses = {}
    if isinstance(data, list):  # World English Bible
        parts = {}
        for element in data:
            if element["type"] not in VERSE_TEXT_TYPES:
                continue
            key = (element["chapterNumber"], element["verseNumber"])
            parts.setdefault(key, []).append(element["value"])
        for key, p in parts.items():
            verses[key] = " ".join(" ".join(p).split())
    else:  # chapter -> verse -> text (see split_bible.py)
        for chapter, vs in data.items():
            if not chapter.isdigit():
                continue  # CPDV's prologues ("P") can't be typed in as a chapter number
            for verse, text in vs.items():
                verses[(int(chapter), int(verse))] = " ".join(text.split())
    return verses


def read_version(version):
    return {p.stem: read_book(p) for p in sorted((BIBLES / version).glob("*.json"))}


# Vulgate psalm -> the Hebrew-numbered verses it holds, as (psalm, first, last).
# None for "last" means to the end of that psalm.
def vulgate_psalm_segments(n):
    if n <= 8 or n >= 148:
        return [(n, 1, None)]
    if n == 9:
        return [(9, 1, None), (10, 1, None)]
    if n <= 112:
        return [(n + 1, 1, None)]
    if n == 113:
        return [(114, 1, None), (115, 1, None)]
    if n == 114:
        return [(116, 1, 9)]
    if n == 115:
        return [(116, 10, None)]
    if n <= 145:
        return [(n + 1, 1, None)]
    if n == 146:
        return [(147, 1, 11)]
    return [(147, 12, None)]  # 147


def words(text):
    return set(re.findall(r"[a-z]+", text.lower()))


def psalm_refs(book_verses, web_psalms):
    """(chapter, verse) -> World English Bible (chapter, verse), or None, for Vulgate-numbered Psalms."""
    web_counts = {}
    for (c, v) in web_psalms:
        web_counts[c] = max(web_counts.get(c, 0), v)
    refs = {}
    chapters = sorted({c for c, _ in book_verses})
    for c in chapters:
        own = sorted(v for ch, v in book_verses if ch == c)
        targets = []
        for psalm, first, last in vulgate_psalm_segments(c):
            last = last or web_counts.get(psalm, 0)
            targets += [(psalm, v) for v in range(first, last + 1)]

        # The Vulgate often counts a psalm's title as verse 1 (or 1-2) where
        # the World English Bible doesn't. Try skipping 0-2 leading verses and
        # keep whichever lines the wording up best.
        def overlap(skip):
            total = 0
            for i, v in enumerate(own):
                j = i - skip
                if 0 <= j < len(targets) and targets[j] in web_psalms:
                    total += len(words(book_verses[(c, v)]) & words(web_psalms[targets[j]]))
            return total
        skip = max(range(3), key=lambda k: (overlap(k), -k))
        for i, v in enumerate(own):
            j = i - skip
            refs[(c, v)] = targets[j] if 0 <= j < len(targets) else None
    return refs


def daniel_refs(book_verses):
    """Daniel 3:24-90 (the Prayer of Azariah and Song of the Three) aren't in the World
    English Bible, so Vulgate 3:91-97 is its 3:24-30 and 3:98-100 its 4:1-3; Vulgate
    chapter 4 is its 4:4-37. Chapters 13-14 (Susanna, Bel) keep their own numbers."""
    refs = {}
    for (c, v) in book_verses:
        if c == 3 and 24 <= v <= 90:
            refs[(c, v)] = None
        elif c == 3 and 91 <= v <= 97:
            refs[(c, v)] = (3, v - 67)
        elif c == 3 and v >= 98:
            refs[(c, v)] = (4, v - 97)
        elif c == 4:
            refs[(c, v)] = (4, v + 3)
    return refs


def build(version, web_books):
    books = read_version(version)
    entries = []
    for book, verses in books.items():
        testament = "Old Testament" if book in OLD_TESTAMENT else "New Testament"
        area = next(a for a, bs in AREAS.items() if book in bs)
        refs = {}
        if version in VULGATE_NUMBERING and book == "psalms":
            refs = psalm_refs(verses, web_books["psalms"])
        elif version in VULGATE_NUMBERING and book == "daniel":
            refs = daniel_refs(verses)
        for (chapter, verse), text in verses.items():
            if chapter > MAX_REFERENCE_NUMBER or verse > MAX_REFERENCE_NUMBER:
                continue
            if len(text.split(" ")) < MIN_WORDS:
                continue
            entry = {"testament": testament, "area": area, "book": book,
                     "chapter": chapter, "verse": verse, "text": text}
            # R18 verses are hidden in-game unless R18 mode is on.
            if is_mature_text(text):
                entry["mature"] = True
            if (chapter, verse) in refs and refs[(chapter, verse)] != (chapter, verse):
                ref = refs[(chapter, verse)]
                entry["ref"] = list(ref) if ref else None
            entries.append(entry)
    return sorted(entries, key=lambda x: (x["book"], x["chapter"], x["verse"]))


def write(path, entries):
    # One verse per line: about half the size of an indented file, still diffable.
    body = ",\n".join(json.dumps(e, ensure_ascii=False) for e in entries)
    Path(path).write_text("[\n" + body + "\n]\n", encoding="utf-8")


def main():
    web_books = read_version("web")
    for version, out in VERSIONS.items():
        entries = build(version, web_books)
        write(out, entries)
        print(f"{version}: {len(entries)} verses -> {out}")
        if WEB_SCRIPTS.is_dir():
            shutil.copyfile(out, WEB_SCRIPTS / out)
    if WEB_SCRIPTS.is_dir():
        print(f"copied to {WEB_SCRIPTS}")


if __name__ == "__main__":
    main()
