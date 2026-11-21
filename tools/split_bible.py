"""Split a Bible's JSON into one file per book under tools/bibles/<version>/.

sort.py reads those book files to build each version's bible_sections file.
The World English Bible files (tools/bibles/web/) come from TehShrike's
world-english-bible project in their own format; every other version uses the
simple format this script writes:

    {"1": {"1": "In the beginning...", "2": "..."}, "2": {...}}   # chapter -> verse -> text

Sources it reads:
  * a JSON object of book name -> chapter -> verse -> text (cpdv.json)
  * a folder with Books.json and per-book files, like Arul John's Bible-kjv repo
    ({"book": "Genesis", "chapters": [{"chapter": "1", "verses": [{"verse": "1", "text": "..."}]}]})
  * a folder of per-book chapter -> verse -> text files named like
    "OT-08_Ruth.json", like yoarikso's douayrheimsbible repo (its douayrheims-json folder)

Run from the repo root:
    python tools/split_bible.py kjv path/to/Bible-kjv-master
    python tools/split_bible.py dr path/to/douayrheimsbible/douayrheims-json
    python tools/split_bible.py cpdv path/to/cpdv.json
"""
import json
import re
import sys
from pathlib import Path

# Book names some sources use for the same book key.
ALIASES = {"songofsongs": "songofsolomon"}


def book_key(name):
    """'1 Samuel' -> '1samuel', 'Song of Solomon' -> 'songofsolomon'."""
    key = re.sub(r"[^a-z0-9]", "", name.lower())
    return ALIASES.get(key, key)


def clean(text):
    # CPDV marks the speakers in Song of Solomon ("<I>Bride:</I>"): headings,
    # not Bible text.
    text = re.sub(r"<I>.*?</I>", "", text)
    text = re.sub(r"<[^>]+>", "", text)
    text = text.replace("`", "'").replace(" ", " ")
    return " ".join(text.split())


def read_source(path):
    """Book name -> {chapter: {verse: text}} in Bible order."""
    path = Path(path)
    if path.is_dir() and (path / "Books.json").exists():
        books = json.loads((path / "Books.json").read_text(encoding="utf-8"))
        out = {}
        for name in books:
            data = json.loads((path / (name.replace(" ", "") + ".json")).read_text(encoding="utf-8-sig"))
            out[data["book"]] = {c["chapter"]: {v["verse"]: v["text"] for v in c["verses"]}
                                 for c in data["chapters"]}
        return out
    if path.is_dir():
        out = {}
        for file in sorted(path.glob("*_*.json")):
            name = file.stem.split("_", 1)[1].replace("-", " ")
            out[name] = json.loads(file.read_text(encoding="utf-8-sig"))
        return out
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_book(path, chapters):
    # Only numbered chapters with verses; sources can carry extra keys
    # (douayrheimsbible's "charset"). CPDV prologues ("P") are kept and
    # skipped later by sort.py.
    chapters = {c: v for c, v in chapters.items() if isinstance(v, dict) and v}
    # One verse per line keeps the files readable and diffs small.
    lines = ["{"]
    for ci, (chapter, verses) in enumerate(chapters.items()):
        lines.append(f"    {json.dumps(str(chapter))}: {{")
        items = list(verses.items())
        for vi, (verse, text) in enumerate(items):
            comma = "," if vi < len(items) - 1 else ""
            lines.append(f"        {json.dumps(str(verse))}: {json.dumps(clean(text), ensure_ascii=False)}{comma}")
        lines.append("    }" + ("," if ci < len(chapters) - 1 else ""))
    lines.append("}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    version, source = sys.argv[1], sys.argv[2]
    out_dir = Path("tools/bibles") / version
    out_dir.mkdir(parents=True, exist_ok=True)
    books = read_source(source)
    for name, chapters in books.items():
        write_book(out_dir / f"{book_key(name)}.json", chapters)
    print(f"wrote {len(books)} books to {out_dir}")


if __name__ == "__main__":
    main()
