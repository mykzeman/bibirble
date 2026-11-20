#pragma once

#include <string>
#include <vector>

// The Bible translations players can pick in Settings. Each one's verses are
// built by tools/sort.py into its own file (embedded like the others). Keep in
// sync with VERSIONS in tools/sort.py and Bibirble-web's scripts/versions.js.
struct BibleVersion {
    const char* id;        // saved in settings; "web" is the default
    const char* name;      // "World English Bible"
    const char* shortName; // "WEB": shown in share text
    const char* file;      // bible_sections*.json, built by tools/sort.py
    const wchar_t* resource;  // Bibirble.rc RCDATA name (Windows)
};

inline const std::vector<BibleVersion>& BibleVersions() {
    static const std::vector<BibleVersion> versions = {
        {"web", "World English Bible", "WEB", "bible_sections.json", L"BIBLE_SECTIONS_JSON"},
        {"kjv", "King James Version", "KJV", "bible_sections_kjv.json", L"BIBLE_KJV_JSON"},
        {"dr", "Douay-Rheims", "DRA", "bible_sections_dr.json", L"BIBLE_DR_JSON"},
        {"cpdv", "Catholic Public Domain Version", "CPDV", "bible_sections_cpdv.json", L"BIBLE_CPDV_JSON"},
    };
    return versions;
}

// The version with this id, or the World English Bible if there's none.
inline const BibleVersion& FindBibleVersion(const std::string& id) {
    for (const auto& v : BibleVersions()) {
        if (id == v.id) return v;
    }
    return BibleVersions().front();
}
