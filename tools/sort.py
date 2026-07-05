from pathlib import Path
import json
dir_path = Path("tools/")
file_list = list(dir_path.glob("*.json"))
big_list = []
OLD_TESTIMENT=["genesis","exodus","leviticus","numbers","deuteronomy","joshua","judges","ruth","1samuel","2samuel","1kings","2kings","1chronicles","2chronicles","ezra","nehemiah","esther","job","psalms","proverbs","ecclesiastes","songofsolomon","isaiah","jeremiah","lamentations","ezekiel","daniel","hosea","joel","amos","obadiah","jonah","micah","nahum","habakkuk","zephaniah","haggai","zechariah","malachi"]
AREAS={
    "Torah":OLD_TESTIMENT[0:5],
    "Historical":["joshua","judges","1samuel","2samuel","1kings","2kings","1chronicles","2chronicles","nehemiah"],
    "Poems":["psalms","proverbs","ecclesiastes","songofsolomon","lamentations"],
    "Small stories":["job","esther","jonah","ruth","ezra"],
    "Prophets Major":["isaiah","jeremiah","ezekiel","daniel"],
    "Prophets Minor":["hosea","joel","amos","obadiah","micah","nahum","habakkuk","zephaniah","haggai","zechariah","malachi"],
    "Gospel":["matthew","mark","luke","john"],
    "Acts from Hebrews":["acts","hebrews"],
    "Pauls letters":["romans","1corinthians","2corinthians","galatians","ephesians","philippians","colossians","1thessalonians","2thessalonians","1timothy","2timothy","titus","philemon"],
    "Peter letters":["1peter","2peter"],
    "James and Jude":["james","jude"],
    "John Letters and Visions":["1john","2john","3john","revelation"]
}

# Element types that carry verse text. "line text" covers poetic passages
# (prophecies, blessings, curses, psalms) which the source renders as verse
# lines rather than paragraph prose -- some verses (e.g. Genesis 3:15) are
# made up entirely of "line text" elements with no "paragraph text" at all.
TEXT_TYPES = {"paragraph text", "line text"}

for file_path in file_list:
    with open(file_path, 'r', encoding='utf-8') as f:
        book = file_path.stem
        data = json.load(f)
        testament = "Old Testament" if book in OLD_TESTIMENT else "New Testament"
        book_area = ""
        for area, books in AREAS.items():
            if book in books:
                book_area = area
                break

        current_key = None
        current_words = []

        def flush():
            if current_key is None or not current_words:
                return
            text = " ".join(current_words).strip()
            # Require 7+ words in the fully-assembled verse (paragraph text
            # plus any poetic line text), not just the first chunk -- so a
            # short opening line like "He said," doesn't cause a multi-line
            # poetic verse to be dropped before its later lines are counted.
            if len(text.split()) < 7:
                return
            chapter, verse = current_key
            big_list.append({
                "testament": testament,
                "area": book_area,
                "book": book,
                "chapter": chapter,
                "verse": verse,
                "text": text,
            })

        for element in data:
            if element["type"] not in TEXT_TYPES:
                continue

            key = (element["chapterNumber"], element["verseNumber"])
            if key != current_key:
                flush()
                current_key = key
                current_words = []

            value = element["value"].strip()
            if value:
                current_words.append(value)

        flush()

big_list = sorted(big_list, key=lambda x: (x["book"], x["chapter"], x["verse"]))
with open("bible_sections.json", 'w', encoding='utf-8') as f:
    json.dump(big_list, f, ensure_ascii=False, indent=4)
print(f"Total sections saved: {len(big_list)}")
