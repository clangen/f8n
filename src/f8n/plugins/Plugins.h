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

#pragma once

#include <f8n/config.h>
#include <f8n/sdk/IPlugin.h>
#include <f8n/sdk/IPreferences.h>
#include <f8n/debug/debug.h>

#include <functional>
#include <vector>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>

#ifdef WIN32
    #define STDCALL(fp) (__stdcall* fp)()
#else
    #include <dlfcn.h>
    #define STDCALL(fp) (* fp)()
#endif

namespace f8n { namespace plugin {

    class Plugins {
        public:
            static Plugins& Instance();

            template <typename T>
            struct ReleaseDeleter {
                void operator()(T* t) {
                    t->Release();
                }
            };

            template <typename T>
            struct NullDeleter {
                void operator()(T* t) {
                }
            };

            template <class T, class D> void QueryInterface(
                const std::string& functionName,
                std::function<void(f8n::sdk::IPlugin*, std::shared_ptr<T>, const std::string&)> handler)
            {
                std::unique_lock<std::mutex> lock(this->mutex);

                typedef T* STDCALL(PluginInterfaceCall);

                for (std::shared_ptr<Descriptor> descriptor : this->plugins) {
                    if (functionName == "GetPlugin" || prefs->GetBool(descriptor->key.c_str(), true)) { /* enabled */
                        PluginInterfaceCall funcPtr =
#ifdef WIN32
                            (PluginInterfaceCall) GetProcAddress((HMODULE)(descriptor->nativeHandle), functionName.c_str());
#else
                            (PluginInterfaceCall)dlsym(descriptor->nativeHandle, functionName.c_str());
#endif
                        if (funcPtr) {
                            T* result = funcPtr();

                            if (result) {
                                handler(descriptor->plugin, std::shared_ptr<T>(result, D()), descriptor->filename);
                            }
                        }
                    }
                }
            }

            template <class T, class D> std::vector<std::shared_ptr<T> > QueryInterface(const std::string& functionName) {
                std::vector<std::shared_ptr<T> > plugins;

                QueryInterface<T, D>(
                    functionName,
                    [&plugins](
                        f8n::sdk::IPlugin* unused,
                        std::shared_ptr<T> plugin,
                        const std::string& fn)
                        {
                            plugins.push_back(plugin);
                        });

                return plugins;
            }

            template <class T> void QueryFunction(
                const std::string& functionName,
                std::function<void(f8n::sdk::IPlugin*, T)> handler)
            {
                std::unique_lock<std::mutex> lock(this->mutex);

                for (std::shared_ptr<Descriptor> descriptor : this->plugins) {
                    if (prefs->GetBool(descriptor->key.c_str(), true)) { /* if enabled by prefs */
                        T funcPtr =
#ifdef WIN32
                            (T) GetProcAddress((HMODULE)(descriptor->nativeHandle), functionName.c_str());
#else
                            (T)dlsym(descriptor->nativeHandle, functionName.c_str());
#endif
                        if (funcPtr) {
                            handler(descriptor->plugin, funcPtr);
                        }
                    }
                }
            }

        private:
            struct Descriptor {
                f8n::sdk::IPlugin* plugin;
                void* nativeHandle;
                std::string filename;
                std::string key;
            };

            Plugins();
            ~Plugins();
            void LoadPlugins();

            std::vector<std::shared_ptr<Descriptor> > plugins;
            std::mutex mutex;
            std::shared_ptr<f8n::sdk::IPreferences> prefs;
    };
} }
