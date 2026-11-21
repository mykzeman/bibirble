#include "EmbeddedData.h"
#include "BibleData.h"

#include <cstring>
#include <fstream>
#include <sstream>

#include <zlib.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Linux: CMake generates EmbeddedDataBlob.cpp, whose .incbin copies the
// Bible versions' verse files and icons/bibirble.png into the binary between these
// symbols, and defines BIBIRBLE_HAS_INCBIN, so the Linux build is a single
// file too.
#if !defined(_WIN32) && defined(BIBIRBLE_HAS_INCBIN)
// Every Bible version (bibles/<id>.json.gz), listed by the generated file.
struct EmbeddedBible { const char* id; const char* start; const char* end; };
extern const EmbeddedBible kEmbeddedBibles[];
extern const int kEmbeddedBibleCount;
extern "C" const char bibirble_icon_start[];
extern "C" const char bibirble_icon_end[];
extern "C" const char bibirble_story_start[];
extern "C" const char bibirble_story_end[];
extern "C" const char bibirble_words_start[];
extern "C" const char bibirble_words_end[];
#endif

namespace {

#ifdef _WIN32
// Must match the resource name/type in Bibirble.rc.
bool ReadEmbeddedResource(std::string& out, const wchar_t* name = L"BIBLE_SECTIONS_JSON") {
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC res = FindResourceW(module, name, MAKEINTRESOURCEW(10) /* RT_RCDATA */);
    if (!res) return false;
    HGLOBAL handle = LoadResource(module, res);
    if (!handle) return false;
    const void* data = LockResource(handle);
    const DWORD size = SizeofResource(module, res);
    if (!data || size == 0) return false;
    out.assign(static_cast<const char*>(data), size);
    return true;
}
#endif

// Unpacks gzip data in place (the Bible versions ship as .json.gz). Data
// that isn't gzipped (a plain .json) is left as it is.
bool Gunzip(std::string& data) {
    if (data.size() < 2 || static_cast<unsigned char>(data[0]) != 0x1f ||
        static_cast<unsigned char>(data[1]) != 0x8b) {
        return true;
    }
    z_stream z;
    std::memset(&z, 0, sizeof(z));
    if (inflateInit2(&z, 16 + MAX_WBITS) != Z_OK) return false;
    z.next_in = reinterpret_cast<Bytef*>(&data[0]);
    z.avail_in = static_cast<uInt>(data.size());
    std::string out;
    char buffer[1 << 16];
    int ret;
    do {
        z.next_out = reinterpret_cast<Bytef*>(buffer);
        z.avail_out = sizeof(buffer);
        ret = inflate(&z, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&z);
            return false;
        }
        out.append(buffer, sizeof(buffer) - z.avail_out);
    } while (ret != Z_STREAM_END);
    inflateEnd(&z);
    data.swap(out);
    return true;
}

}  // namespace

bool ReadBibleDataText(const BibleVersion& version, std::string& out) {
    out.clear();

    const std::string resolvedPath = BibleData::ResolveDataFilePath(version.file);
    if (!resolvedPath.empty()) {
        std::ifstream ifs(resolvedPath, std::ios::binary);
        if (ifs) {
            std::ostringstream ss;
            ss << ifs.rdbuf();
            out = ss.str();
            if (!out.empty() && Gunzip(out)) return true;
        }
    }

#ifdef _WIN32
    return ReadEmbeddedResource(out, version.resource) && Gunzip(out);
#elif defined(BIBIRBLE_HAS_INCBIN)
    for (int i = 0; i < kEmbeddedBibleCount; ++i) {
        if (std::strcmp(kEmbeddedBibles[i].id, version.id) == 0) {
            out.assign(kEmbeddedBibles[i].start, kEmbeddedBibles[i].end);
            return !out.empty() && Gunzip(out);
        }
    }
    return false;
#else
    return false;
#endif
}

bool ReadEmbeddedIconPng(std::string& out) {
#if !defined(_WIN32) && defined(BIBIRBLE_HAS_INCBIN)
    out.assign(bibirble_icon_start, bibirble_icon_end);
    return !out.empty();
#else
    out.clear();
    return false;
#endif
}

namespace {
// The first of these files that exists and isn't empty.
bool ReadFirstFile(std::initializer_list<const char*> paths, std::string& out) {
    for (const char* path : paths) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) continue;
        std::ostringstream ss;
        ss << ifs.rdbuf();
        out = ss.str();
        if (!out.empty()) return true;
    }
    out.clear();
    return false;
}
}  // namespace

bool ReadStoryMapText(std::string& out) {
    if (ReadFirstFile({"story_map.json", "../story_map.json", "../../story_map.json",
                       "build/story_map.json", "Release/story_map.json", "Debug/story_map.json"}, out)) {
        return true;
    }

#ifdef _WIN32
    return ReadEmbeddedResource(out, L"STORY_MAP_JSON");
#elif defined(BIBIRBLE_HAS_INCBIN)
    out.assign(bibirble_story_start, bibirble_story_end);
    return !out.empty();
#else
    return false;
#endif
}

bool ReadWordListText(std::string& out) {
    if (ReadFirstFile({"data/words.txt", "../data/words.txt", "../../data/words.txt", "words.txt"}, out)) {
        return true;
    }

#ifdef _WIN32
    return ReadEmbeddedResource(out, L"WORD_LIST_TXT");
#elif defined(BIBIRBLE_HAS_INCBIN)
    out.assign(bibirble_words_start, bibirble_words_end);
    return !out.empty();
#else
    return false;
#endif
}
