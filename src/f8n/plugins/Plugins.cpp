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

#include <f8n/plugins/Plugins.h>
#include <f8n/preferences/Preferences.h>
#include <f8n/sdk/IPlugin.h>
#include <f8n/debug/debug.h>
#include <f8n/environment/Environment.h>
#include <f8n/utf/conv.h>
#include <iostream>
#include <boost/filesystem.hpp>

static const std::string TAG = "Plugins";
static const std::string PLUGINS_PREFERENCE = "PluginFactoryConfig";
static std::mutex instanceMutex;

using namespace f8n::plugin;
using namespace f8n::sdk;
using namespace f8n::prefs;
using namespace f8n::utf;
using namespace f8n::env;
using namespace f8n;

#ifdef WIN32
typedef f8n::sdk::IPlugin* STDCALL(CallGetPlugin);
static void closeNativeHandle(void* dll) { FreeLibrary((HMODULE)dll); }
#else
typedef f8n::sdk::IPlugin* (*CallGetPlugin)();
static void closeNativeHandle(void* dll) { dlclose(dll); }
#endif

Plugins& Plugins::Instance() {
    std::unique_lock<std::mutex> lock(instanceMutex);

    static Plugins* instance = NULL;

    if (instance == NULL) {
        instance = new Plugins();
    }

    return *instance;
}

Plugins::Plugins() {
    this->prefs = Preferences::ForComponent(PLUGINS_PREFERENCE);
    this->LoadPlugins();
}

Plugins::~Plugins() {
    for (std::shared_ptr<Descriptor> plugin : this->plugins) {
        plugin->plugin->Release();
        closeNativeHandle(plugin->nativeHandle);
    }
    plugins.clear();
}

void Plugins::LoadPlugins() {
#ifdef WIN32
    {
        std::wstring wpath = u8to16(GetPluginDirectory());
        SetDllDirectory(wpath.c_str());
    }
#endif

    std::string pluginDir(GetPluginDirectory());
    boost::filesystem::path dir(pluginDir);

    try {
        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator file(dir); file != end; file++) {
            if (boost::filesystem::is_regular(file->status())){
                std::string filename(file->path().string());

                std::shared_ptr<Descriptor> descriptor(new Descriptor());
                descriptor->filename = filename;
                descriptor->key = boost::filesystem::path(filename).filename().string();

#ifdef WIN32
                /* if the file ends with ".dll", we'll try to load it*/
                if (filename.substr(filename.size() - 4) == ".dll") {
                    HMODULE dll = LoadLibrary(u8to16(filename).c_str());
                    if (dll != NULL) {
                        /* every plugin has a "GetPlugin" method. */
                        CallGetPlugin getPluginCall = (CallGetPlugin) GetProcAddress(dll, "GetPlugin");

                        if (getPluginCall) {
                            /* exists? check the version, and add it! */
                            auto plugin = getPluginCall();
                            if (plugin->SdkVersion() == f8n::env::GetSdkVersion()) {
                                descriptor->plugin = plugin;
                                descriptor->nativeHandle = dll;
                                this->plugins.push_back(descriptor);
                            }
                            else {
                                FreeLibrary(dll);
                            }
                        }
                        else {
                            /* otherwise, free nad move on */
                            FreeLibrary(dll);
                        }
                    }
                }
#else
    #ifdef __APPLE__
                if (filename.substr(filename.size() - 6) == ".dylib") {
                    int openFlags = RTLD_LOCAL;
    #else
                if (filename.substr(filename.size() - 3) == ".so") {
                    int openFlags = RTLD_NOW;
    #endif
                    void* dll = NULL;

                    try {
                        dll = dlopen(filename.c_str(), openFlags);
                    }
                    catch (...) {
                        f8n::debug::err(TAG, "exception while loading plugin " + filename);
                        continue;
                    }

                    if (!dll) {
                        char *err = dlerror();
                        f8n::debug::err(
                            TAG,
                            "could not load shared library " + filename +
                            " error: " + std::string(err));
                    }
                    else {
                        CallGetPlugin getPluginCall;
                        *(void **)(&getPluginCall) = dlsym(dll, "GetPlugin");

                        if (getPluginCall) {
                            auto plugin = getPluginCall();
                            if (plugin->SdkVersion() == f8n::env::SdkVersion) {
                                f8n::debug::info(TAG, "loaded: " + filename);
                                descriptor->plugin = getPluginCall();
                                descriptor->nativeHandle = dll;
                                this->plugins.push_back(descriptor);
                            }
                            else {
                                dlclose(dll);
                            }
                        }
                        else {
                            dlclose(dll);
                        }
                    }
                }
#endif
            }
        }
    }
    catch(...) {
    }
}
