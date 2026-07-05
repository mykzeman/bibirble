#pragma once

#include <string>
#include <vector>

// A single Bible reference used by Story Mode content, resolved against
// BibleData at play time to get the actual verse text.
struct StoryVerseRef {
    std::string book;
    int chapter = 0;
    int verse = 0;
};

struct StoryChapter {
    std::string title;
    std::vector<StoryVerseRef> verses;
};

// One of the 10 major sections of the Bible's story arc. Sections without
// authored chapters yet are "coming soon" placeholders (see story_sections.json).
struct StorySection {
    int id = 0;
    std::string key;
    std::string title;
    std::string subtitle;
    std::string description;
    double mapX = 0.5;
    double mapY = 0.5;
    std::vector<StoryChapter> chapters;
    bool hasBonusChapter = false;
    StoryChapter bonusChapter;
};

class StoryData {
public:
    bool loadData(const std::string& filePath);
    static std::string ResolveDataFilePath(const std::string& preferredPath = "");

    const std::vector<StorySection>& getSections() const { return m_sections; }
    const StorySection* findSection(int sectionId) const;
    bool isLoaded() const { return !m_sections.empty(); }

private:
    std::vector<StorySection> m_sections;
};
