#include "StoryData.h"

#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string StoryData::ResolveDataFilePath(const std::string& preferredPath) {
    std::vector<std::string> candidates;
    if (!preferredPath.empty()) {
        candidates.push_back(preferredPath);
    }

    const std::vector<std::string> defaults = {
        "story_sections.json",
        "./story_sections.json",
        "../story_sections.json",
        "Release/story_sections.json",
        "./Release/story_sections.json",
        "../Release/story_sections.json",
        "Debug/story_sections.json",
        "./Debug/story_sections.json",
        "../Debug/story_sections.json",
        "build/story_sections.json",
        "./build/story_sections.json",
        "../build/story_sections.json",
        "build/Release/story_sections.json",
        "./build/Release/story_sections.json",
        "../build/Release/story_sections.json",
        "build/Debug/story_sections.json",
        "./build/Debug/story_sections.json",
        "../build/Debug/story_sections.json"
    };

    candidates.insert(candidates.end(), defaults.begin(), defaults.end());

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate, std::ios::binary);
        if (file.good()) {
            return candidate;
        }
    }

    return "";
}

namespace {
std::vector<StoryVerseRef> ParseVerseRefs(const json& arr) {
    std::vector<StoryVerseRef> refs;
    for (const auto& obj : arr) {
        StoryVerseRef ref;
        ref.book = obj.value("book", "");
        ref.chapter = obj.value("chapter", 0);
        ref.verse = obj.value("verse", 0);
        refs.push_back(ref);
    }
    return refs;
}
}  // namespace

bool StoryData::loadData(const std::string& filePath) {
    const std::string resolvedPath = ResolveDataFilePath(filePath);
    if (resolvedPath.empty()) {
        return false;
    }

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json root;
        file >> root;

        if (!root.contains("sections") || !root["sections"].is_array()) {
            return false;
        }

        m_sections.clear();
        for (const auto& obj : root["sections"]) {
            StorySection section;
            section.id = obj.value("id", 0);
            section.key = obj.value("key", "");
            section.title = obj.value("title", "");
            section.subtitle = obj.value("subtitle", "");
            section.description = obj.value("description", "");

            if (obj.contains("mapPosition")) {
                section.mapX = obj["mapPosition"].value("x", 0.5);
                section.mapY = obj["mapPosition"].value("y", 0.5);
            }

            if (obj.contains("chapters")) {
                for (const auto& chapterObj : obj["chapters"]) {
                    StoryChapter chapter;
                    chapter.title = chapterObj.value("title", "");
                    if (chapterObj.contains("verses")) {
                        chapter.verses = ParseVerseRefs(chapterObj["verses"]);
                    }
                    section.chapters.push_back(chapter);
                }
            }

            if (obj.contains("bonusChapter") && !obj["bonusChapter"].is_null()) {
                const auto& bonusObj = obj["bonusChapter"];
                section.hasBonusChapter = true;
                section.bonusChapter.title = bonusObj.value("title", "");
                if (bonusObj.contains("verses")) {
                    section.bonusChapter.verses = ParseVerseRefs(bonusObj["verses"]);
                }
            }

            m_sections.push_back(section);
        }
        return true;
    } catch (...) {
        return false;
    }
}

const StorySection* StoryData::findSection(int sectionId) const {
    for (const auto& section : m_sections) {
        if (section.id == sectionId) {
            return &section;
        }
    }
    return nullptr;
}
