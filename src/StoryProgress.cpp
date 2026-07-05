#include "StoryProgress.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

using json = nlohmann::json;

StoryProgress::StoryProgress() {
    m_filePath = ResolveFilePath();
}

std::string StoryProgress::ResolveFilePath() const {
    wxString dir = wxStandardPaths::Get().GetUserDataDir();
    wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName path(dir, "bibirble_story_progress.json");
    return std::string(path.GetFullPath().mb_str());
}

void StoryProgress::Load() {
    std::ifstream file(m_filePath);
    if (!file.is_open()) return;

    try {
        json data;
        file >> data;
        if (!data.contains("sections")) return;

        for (auto& [key, value] : data["sections"].items()) {
            int sectionId = std::atoi(key.c_str());
            StorySectionProgressRecord record;
            record.bonusCompleted = value.value("bonusCompleted", false);
            if (value.contains("chapters")) {
                for (const auto& chapterObj : value["chapters"]) {
                    StoryChapterProgress progress;
                    progress.completed = chapterObj.value("completed", false);
                    progress.perfect = chapterObj.value("perfect", false);
                    record.chapters.push_back(progress);
                }
            }
            m_sections[sectionId] = record;
        }
    } catch (...) {
        // Corrupt or unreadable state file: start fresh rather than crash.
        m_sections.clear();
    }
}

void StoryProgress::Save() const {
    json data;
    json sectionsJson = json::object();

    for (const auto& [sectionId, record] : m_sections) {
        json sectionJson;
        sectionJson["bonusCompleted"] = record.bonusCompleted;
        json chaptersJson = json::array();
        for (const auto& chapter : record.chapters) {
            chaptersJson.push_back({{"completed", chapter.completed}, {"perfect", chapter.perfect}});
        }
        sectionJson["chapters"] = chaptersJson;
        sectionsJson[std::to_string(sectionId)] = sectionJson;
    }

    data["sections"] = sectionsJson;

    std::ofstream file(m_filePath);
    if (!file.is_open()) return;
    file << data.dump(2);
}

StorySectionProgressRecord& StoryProgress::GetOrCreate(int sectionId, int chapterCount) {
    StorySectionProgressRecord& record = m_sections[sectionId];
    if ((int)record.chapters.size() < chapterCount) {
        record.chapters.resize(chapterCount);
    }
    return record;
}

bool StoryProgress::IsSectionUnlocked(int sectionId, int previousSectionChapterCount) const {
    if (sectionId <= 1) return true;
    if (previousSectionChapterCount <= 0) return false;

    return AreAllChaptersCompleted(sectionId - 1, previousSectionChapterCount);
}

bool StoryProgress::IsChapterCompleted(int sectionId, int chapterIndex) const {
    auto it = m_sections.find(sectionId);
    if (it == m_sections.end()) return false;
    if (chapterIndex < 0 || chapterIndex >= (int)it->second.chapters.size()) return false;
    return it->second.chapters[chapterIndex].completed;
}

void StoryProgress::RecordChapterResult(int sectionId, int chapterIndex, int chapterCount, bool perfect) {
    if (chapterIndex < 0 || chapterIndex >= chapterCount) return;

    StorySectionProgressRecord& record = GetOrCreate(sectionId, chapterCount);
    record.chapters[chapterIndex].completed = true;
    record.chapters[chapterIndex].perfect = perfect;
    Save();
}

bool StoryProgress::AreAllChaptersCompleted(int sectionId, int chapterCount) const {
    if (chapterCount <= 0) return false;

    auto it = m_sections.find(sectionId);
    if (it == m_sections.end()) return false;
    if ((int)it->second.chapters.size() < chapterCount) return false;

    for (int i = 0; i < chapterCount; ++i) {
        if (!it->second.chapters[i].completed) return false;
    }
    return true;
}

bool StoryProgress::IsSectionAced(int sectionId, int chapterCount) const {
    if (chapterCount <= 0) return false;

    auto it = m_sections.find(sectionId);
    if (it == m_sections.end()) return false;
    if ((int)it->second.chapters.size() < chapterCount) return false;

    for (int i = 0; i < chapterCount; ++i) {
        const auto& chapter = it->second.chapters[i];
        if (!chapter.completed || !chapter.perfect) return false;
    }
    return true;
}

bool StoryProgress::IsBonusCompleted(int sectionId) const {
    auto it = m_sections.find(sectionId);
    if (it == m_sections.end()) return false;
    return it->second.bonusCompleted;
}

void StoryProgress::MarkBonusCompleted(int sectionId) {
    StorySectionProgressRecord& record = m_sections[sectionId];
    record.bonusCompleted = true;
    Save();
}
