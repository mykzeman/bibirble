"""Builds each Bible version's verse file from the per-book files in tools/bibles/<version>/.

The versions are listed in tools/bible_versions.json. For each one this writes
bibles/<id>.json.gz (embedded in the program), and when the web repo is next to
this one, ../Bibirble-web/scripts/bibles/<id>.json.gz plus a plain
../Bibirble-web/scripts/bible_sections.json for the World English Bible (the
default, so it loads on any browser). It also generates the version lists:
src/BibleVersions.h, bibles/bibles.rc and ../Bibirble-web/scripts/versions.js.

Run from the repo root:  python tools/sort.py

Each verse is {testament, area, book, chapter, verse, text}, plus:
  "mature": true  R18 content, hidden unless R18 mode is on.
  "ref": [c, v]   the verse's place in the World English Bible's numbering when
                  this version numbers it differently (the Douay-Rheims, CPDV
                  and Septuagints number most Psalms one lower). "ref": null
                  means the verse has no World English Bible match. Story Mode,
                  Useful verses and Christmas verses look verses up by this
                  standard reference, so they find the same passage in every
                  version. Players still see and guess the version's own numbers.
"""
import gzip
import json
import re
from pathlib import Path

from content_filters import is_mature_text

BIBLES = Path("tools/bibles")
OUT = Path("bibles")
WEB_SCRIPTS = Path("../Bibirble-web/scripts")
VERSIONS = json.loads(Path("tools/bible_versions.json").read_text(encoding="utf-8"))
DEFAULT_VERSION = "web"

OLD_TESTAMENT = ["genesis", "exodus", "leviticus", "numbers", "deuteronomy", "joshua", "judges", "ruth",
                 "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles", "ezra", "nehemiah",
                 "1esdras", "2esdras", "tobit", "judith", "esther", "esthergreek", "job", "psalms", "psalm151",
                 "prayerofmanasseh", "proverbs", "ecclesiastes", "songofsolomon", "wisdom", "sirach",
                 "psalmsofsolomon", "isaiah", "jeremiah", "lamentations", "baruch", "letterofjeremiah", "ezekiel",
                 "daniel", "danielgreek", "songofthethree", "susanna", "bel", "hosea", "joel", "amos", "obadiah",
                 "jonah", "micah", "nahum", "habakkuk", "zephaniah", "haggai", "zechariah", "malachi",
                 "1maccabees", "2maccabees", "3maccabees", "4maccabees"]
# Must match getBookArea() in src/BibleData.cpp and AREAS in Bibirble-web's scripts/books.js.
# The deuterocanonical / apocryphal books (only some versions have them) join the areas they fit.
AREAS = {
    "Torah": OLD_TESTAMENT[0:5],
    "Historical": ["joshua", "judges", "1samuel", "2samuel", "1kings", "2kings", "1chronicles", "2chronicles",
                   "nehemiah", "1maccabees", "2maccabees", "3maccabees", "4maccabees", "1esdras"],
    "Poems": ["psalms", "proverbs", "ecclesiastes", "songofsolomon", "lamentations", "wisdom", "sirach",
              "psalm151", "prayerofmanasseh", "psalmsofsolomon", "songofthethree"],
    "Small stories": ["job", "esther", "jonah", "ruth", "ezra", "tobit", "judith", "esthergreek", "susanna", "bel"],
    "Prophets Major": ["isaiah", "jeremiah", "ezekiel", "daniel", "baruch", "letterofjeremiah", "danielgreek",
                       "2esdras"],
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


def overlap(a, b):
    return len(words(a) & words(b))


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


def numbers_psalms_like_vulgate(psalms, web_psalms):
    """True if this version's Psalm 22 reads like the World English Bible's Psalm 23
    (the Vulgate and Septuagint join Psalms 9 and 10, so most psalms are one lower)."""
    mine = " ".join(t for (c, _), t in psalms.items() if c == 22)
    web22 = " ".join(t for (c, _), t in web_psalms.items() if c == 22)
    web23 = " ".join(t for (c, _), t in web_psalms.items() if c == 23)
    return bool(mine) and overlap(mine, web23) > overlap(mine, web22)


def build(version, web_books):
    books = read_version(version)
    entries = []
    for book, verses in books.items():
        testament = "Old Testament" if book in OLD_TESTAMENT else "New Testament"
        area = next(a for a, bs in AREAS.items() if book in bs)
        refs = {}
        if book == "psalms" and numbers_psalms_like_vulgate(verses, web_books["psalms"]):
            refs = psalm_refs(verses, web_books["psalms"])
        elif book == "daniel" and (3, 91) in verses:  # the Song of the Three inside chapter 3
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


def to_json(entries):
    # One verse per line: about half the size of an indented file, still diffable.
    return "[\n" + ",\n".join(json.dumps(e, ensure_ascii=False) for e in entries) + "\n]\n"


def write_gz(path, text):
    # mtime=0 so rebuilding unchanged data gives byte-identical files (no git churn).
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as raw, gzip.GzipFile(fileobj=raw, mode="wb", mtime=0, compresslevel=9) as gz:
        gz.write(text.encode("utf-8"))


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding="utf-8") != text:
        path.write_text(text, encoding="utf-8")


def c_string(text):
    return json.dumps(text, ensure_ascii=True)


def write_version_lists():
    header = [
        "#pragma once",
        "",
        "// Generated by tools/sort.py from tools/bible_versions.json. Don't edit by hand.",
        "",
        "#include <string>",
        "#include <vector>",
        "",
        "// The Bible translations players can pick in Settings. Each one's verses are",
        "// built by tools/sort.py into bibles/<id>.json.gz, which is embedded in the",
        "// program (Bibirble.rc / bibles/bibles.rc on Windows, .incbin on Linux).",
        "struct BibleVersion {",
        "    const char* id;         // saved in settings; \"web\" is the default",
        "    const char* name;       // \"World English Bible\"",
        "    const char* shortName;  // \"WEB\": shown in share text",
        "    const char* file;       // bibles/<id>.json.gz",
        "    const wchar_t* resource;  // bibles/bibles.rc RCDATA name (Windows)",
        "};",
        "",
        "inline const std::vector<BibleVersion>& BibleVersions() {",
        "    static const std::vector<BibleVersion> versions = {",
    ]
    for v in VERSIONS:
        header.append(f'        {{{c_string(v["id"])}, {c_string(v["name"])}, {c_string(v["short"])}, '
                      f'{c_string("bibles/" + v["id"] + ".json.gz")}, L{c_string("BIBLE_" + v["id"].upper())}}},')
    header += [
        "    };",
        "    return versions;",
        "}",
        "",
        "// The version with this id, or the World English Bible if there's none.",
        "inline const BibleVersion& FindBibleVersion(const std::string& id) {",
        "    for (const auto& v : BibleVersions()) {",
        "        if (id == v.id) return v;",
        "    }",
        "    return BibleVersions().front();",
        "}",
        "",
    ]
    write_if_changed(Path("src/BibleVersions.h"), "\n".join(header))

    rc = ["// Generated by tools/sort.py: every Bible version, compiled into Bibirble.exe",
          "// (included from Bibirble.rc). Names match BibleVersion::resource."]
    rc += [f'BIBLE_{v["id"].upper()} RCDATA "bibles/{v["id"]}.json.gz"' for v in VERSIONS]
    write_if_changed(OUT / "bibles.rc", "\n".join(rc) + "\n")

    if WEB_SCRIPTS.is_dir():
        js = ["// Generated by bibirble's tools/sort.py from tools/bible_versions.json. Don't edit by hand.",
              "// The Bible translations players can pick in Settings. Only the picked",
              "// version's verses are downloaded: scripts/bibles/<id>.json.gz (gzipped), or",
              "// the plain scripts/bible_sections.json for the World English Bible.",
              "export const VERSIONS = ["]
        for v in VERSIONS:
            file = "bible_sections.json" if v["id"] == DEFAULT_VERSION else f"bibles/{v['id']}.json.gz"
            js.append(f'    {{ id: {c_string(v["id"])}, name: {json.dumps(v["name"], ensure_ascii=False)}, '
                      f'short: {c_string(v["short"])}, file: {c_string(file)} }},')
        js[-1] = js[-1].rstrip(",")
        js.append("];")
        write_if_changed(WEB_SCRIPTS / "versions.js", "\n".join(js) + "\n")


def main():
    web_books = read_version(DEFAULT_VERSION)
    for v in VERSIONS:
        text = to_json(build(v["id"], web_books))
        write_gz(OUT / f"{v['id']}.json.gz", text)
        if WEB_SCRIPTS.is_dir():
            write_gz(WEB_SCRIPTS / "bibles" / f"{v['id']}.json.gz", text)
            if v["id"] == DEFAULT_VERSION:
                write_if_changed(WEB_SCRIPTS / "bible_sections.json", text)
        print(f"{v['id']}: {text.count(chr(10)) - 1} verses")
    write_version_lists()
    print(f"wrote {OUT}/, src/BibleVersions.h" + (f" and {WEB_SCRIPTS}/" if WEB_SCRIPTS.is_dir() else ""))


if __name__ == "__main__":
    main()
