#pragma once

#include <map>
#include <string>
#include <vector>

// Per-chapter completion record within a section.
struct StoryChapterProgress {
    bool completed = false;
    // True if every verse in the chapter was won outright (no round was lost
    // to running out of guesses). Required for the section to be "aced".
    bool perfect = false;
};

struct StorySectionProgressRecord {
    std::vector<StoryChapterProgress> chapters;
    bool bonusCompleted = false;
};

// Persists Story Mode progress (per-section chapter completion, aced status,
// bonus-chapter completion) to a small JSON file, mirroring PersistenceManager.
class StoryProgress {
public:
    StoryProgress();

    void Load();
    void Save() const;

    // Section 1 is always unlocked. A later section unlocks once every
    // chapter of the previous section (with authored content) is completed.
    bool IsSectionUnlocked(int sectionId, int previousSectionChapterCount) const;

    bool IsChapterCompleted(int sectionId, int chapterIndex) const;
    void RecordChapterResult(int sectionId, int chapterIndex, int chapterCount, bool perfect);

    // True once every chapter in the section has been completed (win or lose
    // is fine, as long as all 10 verses were played through).
    bool AreAllChaptersCompleted(int sectionId, int chapterCount) const;

    // A section is "aced" once every chapter in it has been completed with a
    // perfect run (no lost verse rounds). Unlocks the bonus prophecy chapter.
    bool IsSectionAced(int sectionId, int chapterCount) const;

    bool IsBonusCompleted(int sectionId) const;
    void MarkBonusCompleted(int sectionId);

private:
    std::string ResolveFilePath() const;
    StorySectionProgressRecord& GetOrCreate(int sectionId, int chapterCount);

    std::string m_filePath;
    std::map<int, StorySectionProgressRecord> m_sections;
};
