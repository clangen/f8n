#pragma once

#include <ev++.h>

#include <f8n/runtime/MessageQueue.h>

#include <unistd.h>

#include <csignal>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace f8n::runtime;

namespace f8n { namespace daemon {
    enum class Type: int {
        Foreground, Background
    };

    struct Daemon {
        virtual std::string Name() = 0;
        virtual std::string Version() = 0;
        virtual std::string Hash() = 0;
        virtual std::string LockFilename() = 0;
        virtual std::vector<int> GetSignals() = 0;

        virtual void OnSignal(int signal) = 0;
        virtual void OnInit(Type type, f8n::runtime::MessageQueue& messageQueue) = 0;
        virtual void OnDeinit() = 0;
    };
} } /* namespace f8n::daemon */

namespace f8n { namespace daemon { namespace internal {

static void printHelp();
static void handleCommandLine(int argc, char** argv);
static void exitIfRunning();
static pid_t getDaemonPid();
static void startForeground();
static void startDaemon();
static void start();
static void stopDaemon();
static void initUtf8();
static void run();

static const short EVENT_DISPATCH = 1;
static const short EVENT_QUIT = 2;
static const pid_t NOT_RUNNING = (pid_t) -1;
static int pipeFd[2] = { 0 };
static Type type = Type::Background;

static Daemon* instance = nullptr;

class EvMessageQueue: public MessageQueue {
    public:
        void Post(IMessagePtr message, int64_t delayMs) {
            MessageQueue::Post(message, delayMs);

            if (delayMs <= 0) {
                write(pipeFd[1], &EVENT_DISPATCH, sizeof(EVENT_DISPATCH));
            }
            else {
                double delayTs = (double) delayMs / 1000.0;
                loop.once<
                    EvMessageQueue,
                    &EvMessageQueue::DelayedDispatch
                >(-1, ev::TIMER, (ev::tstamp) delayTs, this);
            }
        }

        void DelayedDispatch(int revents) {
            this->Dispatch();
        }

        static void SignalQuit(ev::sig& signal, int revents) {
            write(pipeFd[1], &EVENT_QUIT, sizeof(EVENT_QUIT));
        }

        void ReadCallback(ev::io& watcher, int revents) {
            short type;
            if (read(pipeFd[0], &type, sizeof(type)) == 0) {
                std::cerr << "read() failed.\n";
                exit(EXIT_FAILURE);
            }
            switch (type) {
                case EVENT_DISPATCH: this->Dispatch(); break;
                case EVENT_QUIT: loop.break_loop(ev::ALL); break;
            }
        }

        void Run() {
            io.set(loop);
            io.set(pipeFd[0], ev::READ);
            io.set<EvMessageQueue, &EvMessageQueue::ReadCallback>(this);
            io.start();

            sio.set(loop);
            sio.set<&EvMessageQueue::SignalQuit>();
            sio.start(SIGTERM);

            write(pipeFd[1], &EVENT_DISPATCH, sizeof(EVENT_DISPATCH));

            loop.run(0);
        }

    private:
        ev::dynamic_loop loop;
        ev::io io;
        ev::sig sio;
};

static pid_t getDaemonPid() {
    std::ifstream lock(instance->LockFilename());
    if (lock.good()) {
        int pid;
        lock >> pid;
        if (kill((pid_t) pid, 0) == 0) {
            return pid;
        }
    }
    return NOT_RUNNING;
}

static void stopDaemon() {
    const std::string name = instance->Name();
    pid_t pid = getDaemonPid();
    if (pid == NOT_RUNNING) {
        std::cout << "\n  " << name << " is not running\n\n";
    }
    else {
        std::cout << "\n  stopping " << name << "...";
        kill(pid, SIGTERM);
        int count = 0;
        bool dead = false;
        while (!dead && count++ < 7) { /* try for 7 seconds */
            if (kill(pid, 0) == 0) {
                std::cout << ".";
                std::cout.flush();
                usleep(500000);
            }
            else {
                dead = true;
            }
        }
        std::cout << (dead ? " success" : " failed") << "\n\n";
        if (!dead) {
            exit(EXIT_FAILURE);
        }
    }
}

static void printHelp() {
    std::cout << "\n  "<< instance->Name() << ":\n";
    std::cout << "    --start: start the daemon\n";
    std::cout << "    --foreground: start the in the foreground\n";
    std::cout << "    --stop: shut down the daemon\n";
    std::cout << "    --running: check if the daemon is running\n";
    std::cout << "    --version: print the version\n";
    std::cout << "    --help: show this message\n\n";
}

static void handleCommandLine(int argc, char** argv) {
    if (argc >= 2) {
        const std::string name = instance->Name();
        const std::string version = instance->Version();
        const std::string hash = instance->Hash();

        const std::string command = std::string(argv[1]);
        if (command == "--start") {
            return;
        }
        else if (command == "--foreground") {
            std::cout << "\n  " << name << " starting in the foreground...\n\n";
            type = Type::Foreground;
            return;
        }
        else if (command == "--stop") {
            stopDaemon();
        }
        else if (command == "--version") {
            std::cout << "\n  " << name << " version: " << version << " " << hash << "\n\n";
        }
        else if (command == "--running") {
            pid_t pid = getDaemonPid();
            if (pid == NOT_RUNNING) {
                std::cout << "\n  " << name << " is NOT running\n\n";
            }
            else {
                std::cout << "\n  " << name << " is running with pid " << pid << "\n\n";
            }
        }
        else {
            printHelp();
        }
        exit(EXIT_SUCCESS);
    }
}

static void exitIfRunning() {
    const std::string name = instance->Name();
    if (getDaemonPid() != NOT_RUNNING) {
        std::cerr << "\n " << name << " is already running!\n\n";
        exit(EXIT_SUCCESS);
    }
    std::cerr << "\n  " << name << " is starting...\n\n";
}

static void signalHandler(int signal) {
    instance->OnSignal(signal);
}

static void startBackground() {
    pid_t pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    pid_t sid = setsid();
    if (sid < 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    if (pipe(pipeFd) != 0) {
        std::cerr << "\n  ERROR! couldn't create pipe\n\n";
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    std::ofstream lock(instance->LockFilename());
    if (lock.good()) {
        lock << std::to_string((int) getpid());
    }
}

static void startForeground() {
    if (pipe(pipeFd) != 0) {
        std::cerr << "\n  ERROR! couldn't create pipe\n\n";
        exit(EXIT_FAILURE);
    }

    std::ofstream lock(instance->LockFilename());
    if (lock.good()) {
        lock << std::to_string((int) getpid());
    }
}

static void start() {
    type == Type::Foreground ? startForeground() : startBackground();
}

} } } /* namespace f8n::daemon::internal */

namespace f8n { namespace daemon {
    static void start(int argc, char** argv, Daemon& instance) {
        internal::instance = &instance;
        internal::handleCommandLine(argc, argv);
        internal::exitIfRunning();
        internal::start();
        for (int signal : instance.GetSignals()) {
            std::signal(signal, internal::signalHandler);
        }
        {
            internal::EvMessageQueue messageQueue;
            instance.OnInit(internal::type, messageQueue);
            messageQueue.Run();
        }
        instance.OnDeinit();
    }
} } /* namespace f8n::daemon */
