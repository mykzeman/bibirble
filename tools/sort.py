from pathlib import Path
import json
dir_path = Path("tools/")
file_list = list(dir_path.glob("*.json"))
big_list=[]
i=0
OLD_TESTIMENT=["genesis","exodus","leviticus","numbers","deuteronomy","joshua","judges","ruth","1samuel","2samuel","1kings","2kings","1chronicles","2chronicles","ezra","nehemiah","esther","job","psalms","proverbs","ecclesiastes","songofsolomon","isaiah","jeremiah","lamentations","ezekiel","daniel","hosea","joel","amos","obadiah","jonah","micah","nahum","habakkuk","zephaniah","haggai","zechariah","malachi"]
AREAS={
    "Torah":OLD_TESTIMENT[0:5],
    "Historical":["joshua","judges","1samuel","2samuel","1kings","2kings","1chronicles","2chronicles"],
    "Poems":["psalms","proverbs","ecclesiastes","songofsolomon","lamentations"],
    "Small stories":["job","esther","jonah","ruth","ezra"],
    "Prophets Major":["isaiah","jeremiah","ezekiel","daniel"],
    "Prophets Minor":["hosea","joel","amos","obadiah","micah","nahum","habakkuk","zephaniah","haggai","zechariah","malachi","nehemiah"],
    "Gospel":["matthew","mark","luke","john"],
    "Acts from Hebrews":["acts","hebrews"],
    "Pauls letters":["romans","1corinthians","2corinthians","galatians","ephesians","philippians","colossians","1thessalonians","2thessalonians","1timothy","2timothy","titus","philemon"],
    "Peter letters":["1peter","2peter"],
    "James and Jude":["james","jude"],
    "John Letters and Visions":["1john","2john","3john","revelation"]
}
for file_path in file_list:
    with open(file_path, 'r', encoding='utf-8') as f:
        book=file_path.stem
        data = json.load(f)
        sectionNumbers=[]
        text=""
        for element in data:
            if element["type"]!="paragraph text":
                continue
            chapter=element["chapterNumber"]
            verse=element["verseNumber"]
            testament="Old Testament" if book in OLD_TESTIMENT else "New Testament"
            for area, books in AREAS.items():
                if book in books:
                    book_area=area
                    break
       
            if element["sectionNumber"] ==1:
                if sectionNumbers==[]:
                    sectionNumbers.append(1)
                    text=element["value"]
                else:
                    sectionNumbers=[]
                    text=""
            else:
                sectionNumbers.append(element["sectionNumber"])
                text+= " " + element["value"]
            words_in_text = text.strip().split(" ")
            allowed=False
            if len(sectionNumbers)>=1 and len(words_in_text)>=7:
                if chapter<99 or verse<99:
                            allowed=True
                            
                            
                if allowed:              
                    big_list.append({
                    "testament": testament,
                    "area": book_area,
                    "book": book,
                    "chapter": chapter,
                    "verse": verse, 
                    "text": text.strip(),})
                    sectionNumbers=[]
                    text=""
                    
                
                    i+=1
big_list=sorted(big_list, key=lambda x: (x["book"], x["chapter"], x["verse"]))
with open("bible_sections.json", 'w', encoding='utf-8') as f:
    json.dump(big_list, f, ensure_ascii=False, indent=4)
print(f"Total sections saved: {len(big_list)}")
# exec(open("stats.py").read()) # Commented out as stats.py is not provided
