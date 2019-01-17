//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2017 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include "Filesystem.h"
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <cctype>

#ifdef WIN32
#include <Windows.h>
#include <io.h>
#define R_OK 0
#else
#define DLLEXPORT
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace f8n { namespace env { namespace fs {

    bool DeleteFile(const std::string& path) {
#ifdef WIN32
        std::wstring path16 = u8to16(path.c_str());
        return DeleteFile(path16.c_str()) == TRUE;
#else
        return std::remove(path.c_str()) == 0;
#endif
    }

    std::string PathSeparator() {
#ifdef WIN32
        return "\\";
#else
        return "/";
#endif
    }

    std::string Canonicalize(const std::string& path) {
#ifdef WIN32
        std::wstring path16 = u8to16(path.c_str());
        std::string result8;
        DWORD size = GetFullPathName(path16.c_str(), 0, 0, nullptr);
        if (size) {
            wchar_t* dest = new wchar_t[size];
            if (GetFullPathName(path16.c_str(), size, dest, nullptr)) {
                result8 = u16to8(dest);
            }
            delete[] dest;
        }
        return result8;
#else
        char realname[_POSIX_PATH_MAX];
        if (realpath(path.c_str(), realname) == 0) {
            return "";
        }
        return std::string(realname);
#endif
    }

    bool CreateDirectory(const std::string& path) {
#ifdef WIN32
        auto path16 = u8to16(path.c_str());
        return SHCreateDirectoryEx(nullptr, path16.c_str(), nullptr) == ERROR_SUCCESS;
#else
        if (Exists(path)) {
            return true;
        }

        for (size_t i = 1; i < path.size(); i++) {
            if (path[i] == '/') {
                std::string sub = path.substr(0, i);
                if (!Exists(sub) && mkdir(sub.c_str(), S_IRWXU)) {
                    return false;
                }
            }
        }

        return mkdir(path.c_str(), S_IRWXU) == 0;
#endif
    }

    inline static bool matchesExtension(
        std::string fn, const std::vector<std::string>& exts)
    {
        std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
        for (auto& ext : exts) {
            if (fn.size() >= ext.size() &&
                fn.rfind(ext) == fn.size() - ext.size())
            {
                return true;
            }
        }
        return false;
    }

    static void findFilesWithExtensions(
        const std::string& path,
        const std::vector<std::string>& exts,
        std::vector<std::string>& target,
        bool recursive)
    {
#ifdef WIN32
        auto path16 = u8to16(path.c_str()) + L"*";
        WIN32_FIND_DATA findData;
        HANDLE handle = FindFirstFile(path16.c_str(), &findData);

        if (handle == INVALID_HANDLE_VALUE) {
            return;
        }

        while (FindNextFile(handle, &findData)) {
            if (!findData.cFileName) {
                continue;
            }
            std::string relPath8 = u16to8(findData.cFileName);
            std::string fullPath8 = path + "\\" + relPath8;
            else if (findData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
                if (recursive && relPath8 != "." && relPath8 != "..") {
                    findFilesWithExtensions(fullPath8 + "\\", exts, target, recursive);
                }
            }
            else if (matchesExtension(fullPath8)) {
                target.push_back(Canonicalize(fullPath8));
            }
        }

        FindClose(handle);
#else
        DIR *dir = nullptr;
        struct dirent *entry = nullptr;

        if (!(dir = opendir(path.c_str()))) {
            return;
        }

        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                if (recursive) {
                    std::string name = entry->d_name;
                    if (name == "." || name == "..") {
                        continue;
                    }
                    findFilesWithExtensions(path + "/" + name, exts, target, recursive);
                }
            }
            else {
                std::string fn = entry->d_name;
                if (matchesExtension(fn, exts)) {
                    std::string fullFn = Canonicalize(path + "/" + fn);
                    target.push_back(fullFn);
                }
            }
        }

        closedir(dir);
#endif
    }

    std::vector<std::string> FindFilesWithExtensions(
        const std::string& path,
        const std::vector<std::string>&& exts,
        bool recursive)
    {
        std::vector<std::string> target;
        findFilesWithExtensions(path, exts, target, recursive);
        return target;
    }

    bool IsFile(const std::string& path) {
#ifdef WIN32
        auto path16 = u8to16(path.c_str());
        /* CAL TODO */
#else
        struct stat s;
        return stat(path.c_str(), &s) == 0 && (s.st_mode & S_IFREG);
#endif
    }

    bool IsDirectory(const std::string& path) {
#ifdef WIN32
        auto path16 = u8to16(path.c_str());
        return GetFileAttributes(path16.c_str()) & FILE_ATTRIBUTE_DIRECTORY;
#else
        struct stat s;
        return stat(path.c_str(), &s) == 0 && (s.st_mode & S_IFDIR);
#endif
    }

    bool Exists(const std::string& path) {
#ifdef WIN32
        auto path16 = u8to16(fn.c_str());
        return _waccess(path16.c_str(), R_OK) != -1;
#else
        return access(path.c_str(), R_OK) != -1;
#endif
    }

    std::string Filename(const std::string& path) {
        if (!IsFile(path)) {
            throw std::runtime_error("invalid file passed to fs::Filename");
        }
        auto sep = PathSeparator();
        size_t lastSlash = path.rfind(sep);
        if (lastSlash != std::string::npos) {
            std::string filepart = path.substr(lastSlash + 1);
            size_t lastDot = filepart.rfind(".");
            return filepart.substr(0, lastDot);
        }
        return "";
    }

    std::string Extension(const std::string& path) {
        if (!IsFile(path)) {
            throw std::runtime_error("invalid file passed to fs::Extension");
        }
        auto sep = PathSeparator();
        size_t lastSlash = path.rfind(sep);
        if (lastSlash != std::string::npos) {
            std::string filepart = path.substr(lastSlash + 1);
            size_t lastDot = filepart.rfind(".");
            if (lastDot != std::string::npos) {
                return filepart.substr(lastDot + 1);
            }
        }
        return "";
    }

} } }
