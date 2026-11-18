#include "EmbeddedData.h"
#include "BibleData.h"

#include <fstream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Linux: CMake generates EmbeddedDataBlob.cpp, whose .incbin copies
// bible_sections.json and icons/bibirble.png into the binary between these
// symbols, and defines BIBIRBLE_HAS_INCBIN, so the Linux build is a single
// file too.
#if !defined(_WIN32) && defined(BIBIRBLE_HAS_INCBIN)
extern "C" const char bibirble_data_start[];
extern "C" const char bibirble_data_end[];
extern "C" const char bibirble_icon_start[];
extern "C" const char bibirble_icon_end[];
#endif

namespace {

#ifdef _WIN32
// Must match the resource name/type in Bibirble.rc.
bool ReadEmbeddedResource(std::string& out) {
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC res = FindResourceW(module, L"BIBLE_SECTIONS_JSON", MAKEINTRESOURCEW(10) /* RT_RCDATA */);
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

}  // namespace

bool ReadBibleDataText(const std::string& preferredPath, std::string& out) {
    out.clear();

    const std::string resolvedPath = BibleData::ResolveDataFilePath(preferredPath);
    if (!resolvedPath.empty()) {
        std::ifstream ifs(resolvedPath, std::ios::binary);
        if (ifs) {
            std::ostringstream ss;
            ss << ifs.rdbuf();
            out = ss.str();
            if (!out.empty()) return true;
        }
    }

#ifdef _WIN32
    return ReadEmbeddedResource(out);
#elif defined(BIBIRBLE_HAS_INCBIN)
    out.assign(bibirble_data_start, bibirble_data_end);
    return !out.empty();
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
