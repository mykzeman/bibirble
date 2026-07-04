#include "PersistenceManager.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

using json = nlohmann::json;

PersistenceManager::PersistenceManager() {
    m_filePath = ResolveFilePath();
}

std::string PersistenceManager::ResolveFilePath() const {
    wxString dir = wxStandardPaths::Get().GetUserDataDir();
    wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName path(dir, "bibirble_state.json");
    return std::string(path.GetFullPath().mb_str());
}

void PersistenceManager::Load() {
    std::ifstream file(m_filePath);
    if (!file.is_open()) return;

    try {
        json data;
        file >> data;
        m_lastPlayedDaily = data.value("lastPlayedDaily", "");
        if (data.contains("lastSeed") && !data["lastSeed"].is_null()) {
            m_lastSeed = data["lastSeed"].get<int64_t>();
            m_hasLastSeed = true;
        }
    } catch (...) {
        // Corrupt or unreadable state file: start fresh rather than crash.
    }
}

void PersistenceManager::Save() const {
    json data;
    data["lastPlayedDaily"] = m_lastPlayedDaily;
    if (m_hasLastSeed) {
        data["lastSeed"] = m_lastSeed;
    }

    std::ofstream file(m_filePath);
    if (!file.is_open()) return;
    file << data.dump(2);
}

void PersistenceManager::SetLastPlayedDaily(const std::string& utcDateString) {
    m_lastPlayedDaily = utcDateString;
    Save();
}

void PersistenceManager::SetLastSeed(int64_t seed) {
    m_lastSeed = seed;
    m_hasLastSeed = true;
    Save();
}
