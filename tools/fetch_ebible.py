"""Download Bible translations from ebible.org (via bible.helloao.org) into per-book files.

Each translation in tools/bible_versions.json that has an "ebible" id is
fetched as one complete.json and split into tools/bibles/<id>/<book>.json
(chapter -> verse -> text, the format split_bible.py writes and sort.py reads).

Run from the repo root:
    python tools/fetch_ebible.py               # every ebible version
    python tools/fetch_ebible.py kjv ylt       # just these
    python tools/fetch_ebible.py --from DIR    # use complete.json files already downloaded
                                               # (named <ebible id>.json, e.g. eng_kjv.json)
"""
import argparse
import json
import re
import urllib.request
from pathlib import Path

API = "https://bible.helloao.org/api/{}/complete.json"

# USFM book codes -> the game's book keys (books.js / BibleData.cpp).
BOOKS = {
    "GEN": "genesis", "EXO": "exodus", "LEV": "leviticus", "NUM": "numbers", "DEU": "deuteronomy",
    "JOS": "joshua", "JDG": "judges", "RUT": "ruth", "1SA": "1samuel", "2SA": "2samuel",
    "1KI": "1kings", "2KI": "2kings", "1CH": "1chronicles", "2CH": "2chronicles", "EZR": "ezra",
    "NEH": "nehemiah", "EST": "esther", "JOB": "job", "PSA": "psalms", "PRO": "proverbs",
    "ECC": "ecclesiastes", "SNG": "songofsolomon", "ISA": "isaiah", "JER": "jeremiah",
    "LAM": "lamentations", "EZK": "ezekiel", "DAN": "daniel", "HOS": "hosea", "JOL": "joel",
    "AMO": "amos", "OBA": "obadiah", "JON": "jonah", "MIC": "micah", "NAM": "nahum",
    "HAB": "habakkuk", "ZEP": "zephaniah", "HAG": "haggai", "ZEC": "zechariah", "MAL": "malachi",
    "MAT": "matthew", "MRK": "mark", "LUK": "luke", "JHN": "john", "ACT": "acts", "ROM": "romans",
    "1CO": "1corinthians", "2CO": "2corinthians", "GAL": "galatians", "EPH": "ephesians",
    "PHP": "philippians", "COL": "colossians", "1TH": "1thessalonians", "2TH": "2thessalonians",
    "1TI": "1timothy", "2TI": "2timothy", "TIT": "titus", "PHM": "philemon", "HEB": "hebrews",
    "JAS": "james", "1PE": "1peter", "2PE": "2peter", "1JN": "1john", "2JN": "2john", "3JN": "3john",
    "JUD": "jude", "REV": "revelation",
    # Deuterocanon / Apocrypha
    "TOB": "tobit", "JDT": "judith", "ESG": "esthergreek", "WIS": "wisdom", "SIR": "sirach",
    "BAR": "baruch", "LJE": "letterofjeremiah", "S3Y": "songofthethree", "SUS": "susanna",
    "BEL": "bel", "DAG": "danielgreek", "1MA": "1maccabees", "2MA": "2maccabees",
    "3MA": "3maccabees", "4MA": "4maccabees", "1ES": "1esdras", "2ES": "2esdras",
    "MAN": "prayerofmanasseh", "PSS": "psalmsofsolomon",
}
# Books left out of the game on purpose.
SKIP = {"PS2"}  # Psalm 151


def verse_text(content):
    """A verse's text: its strings, skipping footnote markers and other objects."""
    parts = []
    for item in content:
        if isinstance(item, str):
            parts.append(item)
        elif isinstance(item, dict) and isinstance(item.get("text"), str) and not item.get("descriptive"):
            parts.append(item["text"])  # words of Jesus, poetry lines (not headings like "BETH")
    return " ".join(" ".join(parts).split())


def split(complete, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.json"):
        old.unlink()
    written = 0
    for book in complete["books"]:
        if book["id"] in SKIP:
            continue
        key = BOOKS.get(book["id"])
        if not key:
            print(f"  skipped unknown book {book['id']} ({book.get('commonName')})")
            continue
        chapters = {}
        for ch in book["chapters"]:
            verses = {}
            for item in ch["chapter"]["content"]:
                if item.get("type") == "verse":
                    text = verse_text(item.get("content", []))
                    if text:
                        verses[str(item["number"])] = text
            if verses:
                chapters[str(ch["chapter"]["number"])] = verses
        lines = ["{"]
        for ci, (chapter, verses) in enumerate(chapters.items()):
            lines.append(f"    {json.dumps(chapter)}: {{")
            items = list(verses.items())
            for vi, (verse, text) in enumerate(items):
                comma = "," if vi < len(items) - 1 else ""
                lines.append(f"        {json.dumps(verse)}: {json.dumps(text, ensure_ascii=False)}{comma}")
            lines.append("    }" + ("," if ci < len(chapters) - 1 else ""))
        lines.append("}")
        (out_dir / f"{key}.json").write_text("\n".join(lines) + "\n", encoding="utf-8")
        written += 1
    return written


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("versions", nargs="*", help="game version ids (default: every ebible version)")
    parser.add_argument("--from", dest="source", metavar="DIR", help="folder of already-downloaded complete.json files")
    args = parser.parse_args()

    table = json.loads(Path("tools/bible_versions.json").read_text(encoding="utf-8"))
    for v in table:
        if not v.get("ebible") or (args.versions and v["id"] not in args.versions):
            continue
        if args.source:
            complete = json.loads((Path(args.source) / f"{v['ebible']}.json").read_text(encoding="utf-8"))
        else:
            with urllib.request.urlopen(API.format(v["ebible"]), timeout=300) as res:
                complete = json.load(res)
        n = split(complete, Path("tools/bibles") / v["id"])
        print(f"{v['id']}: {n} books from {v['ebible']}")


if __name__ == "__main__":
    main()
