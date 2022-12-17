//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004-2020 musikcube team
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

#include <f8n/environment/Environment.h>
#include <f8n/environment/Filesystem.h>
#include <f8n/i18n/Locale.h>
#include <f8n/str/utf.h>
#include <f8n/str/util.h>

#include <cstdlib>
#include <iostream>
#include <fstream>

#ifdef WIN32
    #include <shellapi.h>
#elif __APPLE__
    #include <mach-o/dyld.h>
    #include <unistd.h>
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <limits.h>
#endif

#ifdef __FreeBSD__
    #include <sys/types.h>
    #include <sys/sysctl.h>
#endif

using namespace f8n::utf;
using namespace f8n::prefs;
using namespace f8n::env;

static std::string getDataDirectoryRoot() {
    std::string directory;

#ifdef WIN32
    DWORD bufferSize = GetEnvironmentVariable(L"APPDATA", 0, 0);
    wchar_t *buffer = new wchar_t[bufferSize + 2];
    GetEnvironmentVariable(L"APPDATA", buffer, bufferSize);
    directory.assign(u16to8(buffer));
    delete[] buffer;
#else
    directory = std::string(std::getenv("HOME")) + "/.config/";
#endif

    return directory;
}

namespace f8n { namespace env {
    static const std::string DEFAULT_PREFERENCES = "default";
    static std::string appName = "defaultAppName";
    static int sdkVersion = 15;

    void Initialize(const std::string& appName, int sdkVersion) {
        f8n::env::appName = appName;
        f8n::env::sdkVersion = sdkVersion;

        srand((unsigned int)time(0));

        f8n::i18n::Locale::Instance().Initialize(
            f8n::env::GetApplicationDirectory() + "/locales");
    }

    int GetSdkVersion() {
        return sdkVersion;
    }

    std::shared_ptr<Preferences> GetDefaultPreferences() {
        return Preferences::ForComponent(DEFAULT_PREFERENCES);
    }

    std::string GetPluginDirectory() {
        std::string path(GetApplicationDirectory());
        path.append("/plugins/");
        return path;
    }

    std::string GetApplicationDirectory() {
        std::string result;

#ifdef WIN32
        wchar_t widePath[2048];
        int length = GetModuleFileName(NULL, widePath, 2048);
        if (length != 0 && length < 2048) {
            result.assign(GetPath(f8n::utf::u16to8(widePath).c_str()));
        }
#elif __APPLE__
        char pathbuf[PATH_MAX + 1];
        uint32_t bufsize = sizeof(pathbuf);
        _NSGetExecutablePath(pathbuf, &bufsize);
        result.assign(pathbuf);
        realpath(result.c_str(), pathbuf); /* follow symlink if necessary */
        result.assign(pathbuf);
        size_t last = result.find_last_of("/");
        result = result.substr(0, last); /* remove filename component */
#else
        char pathbuf[PATH_MAX + 1] = { 0 };

#ifdef __FreeBSD__
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;
        size_t bufsize = sizeof(pathbuf);
        sysctl(mib, 4, pathbuf, &bufsize, nullptr, 0);
#elif defined  __OpenBSD__
        int mib[4];
        char** argv;
        size_t len = ARG_MAX;

        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC_ARGS;
        mib[2] = getpid();
        mib[3] = KERN_PROC_ARGV;

        argv = new char* [len];
        if (sysctl(mib, 4, argv, &len, nullptr, 0) < 0) abort();

        fs::path command = fs::absolute(fs::path(fs::u8path(argv[0])));
        realpath(command.u8string().c_str(), pathbuf);
        delete[] argv;
#else
        std::string pathToProc = f8n::str::format("/proc/%d/exe", (int)getpid());
        readlink(pathToProc.c_str(), pathbuf, PATH_MAX);
#endif
        result.assign(pathbuf);
        size_t last = result.find_last_of("/");
        result = result.substr(0, last); /* remove filename component */
#endif

        return result;
    }

    std::string GetHomeDirectory() {
        std::string directory;

    #ifdef WIN32
        DWORD bufferSize = GetEnvironmentVariable(L"USERPROFILE", 0, 0);
        wchar_t *buffer = new wchar_t[bufferSize + 2];
        GetEnvironmentVariable(L"USERPROFILE", buffer, bufferSize);
        directory.assign(u16to8(buffer));
        delete[] buffer;
    #else
        directory = std::string(std::getenv("HOME"));
    #endif

        return directory;
    }

    std::string GetDataDirectory(bool create) {
        std::string directory = getDataDirectoryRoot() + "/" + appName + "/";

        if (create) {
            fs::CreateDirectory(directory);
        }

        return directory;
    }

    std::string GetPath(const std::string &sFile) {
        std::string sPath;
        int length;

    #ifdef WIN32
        wchar_t widePath[2048];
        wchar_t *szFile = NULL;

        length = GetFullPathName(u8to16(sFile).c_str(), 2048, widePath, &szFile);
        if(length != 0 && length < 2048) {
            sPath.assign(u16to8(widePath).c_str());
            if(szFile!=0) {
                std::string sTheFile = u16to8(szFile);
                sPath.assign(sPath.substr(0,length-sTheFile.length()));
            }
        }
        else {
            sPath.assign(sFile);
        }
     #else
        char* szDir;
        sPath.assign(getcwd((char*)szDir, (size_t) length));
     #endif

        return sPath;
    }

    int64_t Checksum(char *data,unsigned int bytes) {
        int64_t sum = 0;
        for(unsigned int i = 0; i < bytes; ++i) {
            char ch = *(data + i);
            sum += (int64_t) ch;
        }
        return sum;
    }

    bool FileToByteArray(const std::string& path, char** target, int& size, bool nullTerminate) {
    #ifdef WIN32
        std::wstring u16fn = u8to16(path);
        FILE* file = _wfopen(u16fn.c_str(), L"rb");
    #else
        FILE* file = fopen(path.c_str(), "rb");
    #endif

        *target = nullptr;
        size = 0;

        if (!file) {
            return false;
        }

        bool success = false;

        if (fseek(file, 0L, SEEK_END) == 0) {
            long fileSize = ftell(file);
            if (fileSize == -1) {
                goto close_and_return;
            }

            if (fseek(file, 0L, SEEK_SET) != 0) {
                goto close_and_return;
            }

            *target = (char*)malloc(sizeof(char) * (fileSize + (nullTerminate ? 1 : 0)));
            size = fread(*target, sizeof(char), fileSize, file);

            if (size == fileSize) {
                if (nullTerminate) {
                    (*target)[size] = 0;
                }

                success = true;
            }
        }

    close_and_return:
        fclose(file);

        if (!success) {
            free(*target);
        }

        return success;
    }

    void OpenFile(const std::string& path) {
    #ifdef WIN32
        ShellExecuteA(nullptr, nullptr, path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    #elif __APPLE__
        std::string command = "open '" + path + "' > /dev/null 2> /dev/null";
        system(command.c_str());
    #else
        std::string command = "xdg-open '" + path + "' > /dev/null 2> /dev/null";
        system(command.c_str());
    #endif
    }

    bool CopyFile(const std::string& from, const std::string& to) {
        if (from.size() && to.size() && from != to) {
            std::ifstream in(from);
            if (in.is_open()) {
                std::ofstream out(to);
                if (out.is_open()) {
                    out << in.rdbuf();
                    return true;
                }
            }
        }
        return false;
    }

} }
