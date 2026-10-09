"""Content filters shared by sort.py and pick_useful.py."""
import re

# Mature (R18) content: sexual content, explicit body references, and
# graphic violence. sort.py tags matching verses with "mature": true; the
# games hide them unless R18 mode is on, and pick_useful.py never puts them
# in the Useful pool. Plain "naked" and "womb" are left
# out on purpose (Job 1:21 is about birth).
MATURE_PATTERNS = [
    # Plural only, and not grief: "the waved breast" is sacrifice meat and
    # "beat their breasts" (Luke 23:48) is mourning.
    r"(?<!beat their )(?<!beating their )(?<!beat his )\bbreasts\b", r"\bnakedness\b", r"\bprostitut", r"\bharlot",
    r"\bwhore", r"\badulter(ess|esses|er|ers|ous)\b", r"\blust", r"\bsexual",
    # "slept with his fathers" means died; "come in to him" (Revelation 3:20)
    # is Jesus at the door.
    r"\b(lie|lay|lain|lying|slept|sleep|sleeps) with\b(?! (his|their|your|my|our) fathers)",
    r"\b(go|goes|went|came|come|comes) in to (her|his wife|your wife|my wife|his neighbor.s wife|a prostitute)\b",
    r"\b(knew|known) her\b", r"\bforeskins?\b", r"\bconcubines?\b", r"\brap(e|ed)\b", r"\bravish",
    r"\bgenitals?\b", r"\bsodomite",
    r"\bexcrement\b", r"\burine\b",
    r"\bflesh of (their|your|his) (sons|daughters)\b", r"\bripped up\b",
    r"\bdash(ed|es)? (in pieces|to pieces|against)\b",
]


def is_mature_text(text):
    lower = text.lower()
    return any(re.search(pat, lower) for pat in MATURE_PATTERNS)
