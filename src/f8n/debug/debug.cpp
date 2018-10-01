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

#include <f8n/debug/debug.h>
#include <functional>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

using namespace f8n;

class log_queue;

static std::vector<std::unique_ptr<f8n::debug::backend>> backends;
static std::thread* thread = nullptr;
static log_queue* queue = nullptr;
static std::recursive_mutex mutex;
static volatile bool cancel = true;

class log_queue {
    public:
        struct log_entry {
            log_entry(debug::level l, const std::string& t, const std::string& m) {
                level = l;
                tag = t;
                message = m;
            }

            debug::level level;
            std::string tag;
            std::string message;
        };

        class stopped_exception {
        };

        log_queue() {
            active = true;
        }

        log_entry* pop_top() {
            std::unique_lock<std::mutex> lock(queue_mutex);
            while ((queue.size() == 0) && (active == true)) {
                wait_for_next_item_condition.wait(lock);
            }

            if (!active) {
                return nullptr;
            }

            log_entry* top = queue.front();
            queue.pop();
            return top;
        }

        bool push(log_entry* f) {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (active) {
                bool was_empty = (queue.size() == 0);
                queue.push(f);

                if (was_empty) {
                    wait_for_next_item_condition.notify_one();
                }

                return true;
            }

            return false;
        }

        void stop() {
            std::unique_lock<std::mutex> lock(queue_mutex);
            active = false;

            while (queue.size() > 0) {
                log_entry* top = queue.front();
                queue.pop();
                delete top;
            }

            wait_for_next_item_condition.notify_all();
        }

    private:
        std::queue<log_entry*> queue;
        std::condition_variable wait_for_next_item_condition;
        std::mutex queue_mutex;
        volatile bool active;
};

static void thread_proc() {
    try {
        while (!cancel) {
            log_queue::log_entry* entry = queue->pop_top();
            if (entry) {
                for (auto& backend : backends) {
                    switch (entry->level) {
                        case f8n::debug::level::verbose:
                            backend->verbose(entry->tag, entry->message);
                            break;
                        case f8n::debug::level::info:
                            backend->info(entry->tag, entry->message);
                            break;
                        case f8n::debug::level::warning:
                            backend->warning(entry->tag, entry->message);
                            break;
                        case f8n::debug::level::error:
                            backend->verbose(entry->tag, entry->message);
                            break;
                    }
                }
                delete entry;
            }
        }
    }
    catch (log_queue::stopped_exception&) {
    }
}

void f8n::debug::start(std::vector<f8n::debug::backend*> backends) {
    std::unique_lock<std::recursive_mutex> lock(mutex);

    if (queue || thread) {
        return;
    }

    for (auto backend : backends) {
        ::backends.push_back(std::unique_ptr<f8n::debug::backend>(backend));
    }

    cancel = false;
    queue = new log_queue();
    thread = new std::thread(std::bind(&thread_proc));
}

void debug::stop() {
    std::unique_lock<std::recursive_mutex> lock(mutex);

    cancel = true;

    if (thread && queue) {
        queue->stop();
        thread->join();

        delete thread;
        thread = nullptr;
        delete queue;
        queue = nullptr;
    }
}

static void enqueue(debug::level level, const std::string& tag, const std::string& string) {
    std::unique_lock<std::recursive_mutex> lock(mutex);

    if (queue) {
        queue->push(new log_queue::log_entry(level, tag, string));
    }
}

void debug::verbose(const std::string& tag, const std::string& string) {
    enqueue(debug::level::verbose, tag, string);
}

void debug::info(const std::string& tag, const std::string& string) {
    enqueue(debug::level::info, tag, string);
}

void debug::warning(const std::string& tag, const std::string& string) {
    enqueue(debug::level::warning, tag, string);
}

void debug::error(const std::string& tag, const std::string& string) {
    enqueue(debug::level::error, tag, string);
}

////////// FileBackend //////////

namespace f8n {

    debug::FileBackend::FileBackend(const std::string& fn)
    : out(fn.c_str()) {

    }

    debug::FileBackend::FileBackend(FileBackend&& fn) {
        this->out.swap(fn.out);
    }

    debug::FileBackend::~FileBackend() {
        if (this->out.is_open()) {
            this->out.flush();
        }
    }

    void debug::FileBackend::verbose(const std::string& tag, const std::string& string) {
        this->out << "[verbose] [" << tag << "] " << string << "\n";
    }

    void debug::FileBackend::info(const std::string& tag, const std::string& string) {
        this->out << "[info] [" << tag << "] " << string << "\n";
    }

    void debug::FileBackend::warning(const std::string& tag, const std::string& string) {
        this->out << "[warning] [" << tag << "] " << string << "\n";
    }

    void debug::FileBackend::error(const std::string& tag, const std::string& string) {
        this->out << "[error] [" << tag << "] " << string << "\n";
    }

}