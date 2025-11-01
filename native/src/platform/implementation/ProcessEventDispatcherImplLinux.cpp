#ifdef __linux__
// https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/4/html/reference_guide/s1-proc-directories#s2-proc-processdirs
#include "platform/IProcessEventDispatcherImplPlatform.h"
#include <memory>
#include <thread>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <string_view>
#include <filesystem>
#include <sys/inotify.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <iconv.h>
#include "Logger.h"

static bool IsStringNumber(const std::string& str) {
    for (char ch : str) {
        if (!(ch >= '0' && ch <= '9')) {
            return false;
        }
    }
    return true;
};

static std::shared_ptr<ProcessInfo> InfoFromPIDString(const char* pid) { // should never return nullptr, add more things as available
    std::string name, path(PATH_MAX, '\0'), proc_dir = "/proc/";
    std::ifstream(proc_dir + pid + "/comm") >> name;
    ssize_t len = readlink((proc_dir + pid + "/exe").c_str(), path.data(), PATH_MAX - 1);
    path.resize(len);
    return std::make_shared<ProcessInfo>(name, path, std::stoul(pid));
}

struct ProcessWatcher {
    ProcessWatcher(IProcessEventDispatcherImplPlatform::Callback enqueue) : enqueue(enqueue), 
                                                                            notify_descriptor(inotify_init1(IN_NONBLOCK)), 
                                                                            event_descriptor(eventfd(0, EFD_NONBLOCK)),
                                                                            watch_descriptor(inotify_add_watch(notify_descriptor, "/proc", IN_CREATE | IN_DELETE)) {
        if(notify_descriptor < 0) {
            event_descriptor >= 0 && close(event_descriptor);
            watch_descriptor >= 0 && close(watch_descriptor);
            throw std::runtime_error("[ProcessWatcher] Failed to initialize inotify file descriptor!"); 
        }
        if(event_descriptor < 0) { 
            close(notify_descriptor);
            watch_descriptor >= 0 && close(watch_descriptor);
            throw std::runtime_error("[ProcessWatcher] Failed to initialize event descriptor!"); 
        }
        if(watch_descriptor < 0) {
            close(notify_descriptor);
            close(event_descriptor);
            throw std::runtime_error("[ProcessWatcher] Failed to initialize watch descriptor!"); 
        }
    }

    ~ProcessWatcher() { stop(); }

    void start() {
        if(poll_thread.joinable()) {
            _RWARN("[ProcessWatcher] Tried to start a ProcessWatcher that's already started!");
            return;
        }

        poll_thread = std::thread([this] {
            pollfd poll_fds[2] = {
                { notify_descriptor, POLLIN, 0 },
                { event_descriptor, POLLIN, 0 },
            };

            constexpr static size_t BUF_LEN = 4096;
            std::vector<uint8_t> buffer(BUF_LEN);            

            for(auto poll_result = poll(poll_fds, 2, -1); poll_result > 0 && (poll_fds[1].revents & POLLIN) == 0; poll_result = poll(poll_fds, 2, -1)) {
                // no inotify activity
                if (!(poll_fds[0].revents & POLLIN)) continue;
                
                ssize_t len = read(notify_descriptor, buffer.data(), buffer.size());
                if(len <= 0) continue;
                ForEachEvent([this](inotify_event& event){
                    if(!event.len || !IsStringNumber(event.name)) return;
                    if(event.mask & IN_CREATE) {
                        // build ProcessInfo
                        enqueue({ EProcessInfoEvent::PROCESS_ADDED, std::reinterpret_pointer_cast<void>(InfoFromPIDString(event.name)) });
                        return;
                    }
                    if(event.mask & IN_DELETE) {
                        enqueue({EProcessInfoEvent::PROCESS_REMOVED, std::reinterpret_pointer_cast<void>(std::make_shared<unsigned long>(std::stoul(event.name)))});
                        return;
                    }
                }, buffer, len);
            }
        });
    }

    void stop() {
        if(event_descriptor >= 0) {
            uint64_t one = 1;
            write(event_descriptor, &one, sizeof(uint64_t));
        }
        if(poll_thread.joinable()) poll_thread.join();
        if(notify_descriptor >= 0) close(notify_descriptor);
        if(watch_descriptor >= 0) close(watch_descriptor);
        notify_descriptor = watch_descriptor = -1;
    }

private:
    IProcessEventDispatcherImplPlatform::Callback enqueue;
    std::thread poll_thread;
    int notify_descriptor;
    int event_descriptor;
    int watch_descriptor;

    static void ForEachEvent(auto&& callback, std::vector<uint8_t>& buffer, ssize_t length) {
        for (uint8_t* ptr = buffer.data(); ptr < buffer.data() + length;) {
            auto* ev = reinterpret_cast<inotify_event*>(ptr);
            callback(*ev);
            ptr += sizeof(inotify_event) + ev->len;
        }
    };
};

struct ProcessEventDispatcherImplLinux : public IProcessEventDispatcherImplPlatform {
    // Inherited via IProcessEventDispatcherImplPlatform
    void startListening() override {
        if(process_watcher) {
            _RWARN("[ProcessEventDispatcherImplLinux] Can't start an already started dispatcher!");
            return;
        }

        process_watcher = std::make_shared<ProcessWatcher>(cb);
        process_watcher->start();
    }

    void stopListening() override {
        process_watcher = nullptr;
    }

    bool isListening() const override {
        return !!process_watcher;
    }

    bool killProcess(unsigned long pid) const override {
        if (kill(pid, SIGKILL) == 0) return true;
        _RERR("Failed to kill process ", pid);
        return false;
    }

    bool systemPrefersDarkMode() const override {
        return true;
    }

    std::vector<uint8_t> getExeIcon(unsigned long pid, const std::wstring& path) const override {
        return std::vector<uint8_t>();
    }

    std::vector<ProcessInfoUpdate> getAllProcessesAsAddedEvent() override {
        auto directory_iterator = std::filesystem::directory_iterator("/proc");
        std::vector<ProcessInfoUpdate> return_vec;
        for(auto& directory_entry : directory_iterator) {
            if(!directory_entry.is_directory()) continue;
            auto path_name = directory_entry.path().filename();
            if(!IsStringNumber(path_name)) continue;
            return_vec.emplace_back(InfoFromPIDString(path_name.c_str()));
        }
        return return_vec;
    }

    void setCallback(Callback cb) override { this->cb = cb; }

    static std::shared_ptr<ProcessEventDispatcherImplLinux> getInstance() {
        if(!instance) instance = std::make_shared<ProcessEventDispatcherImplLinux>();
        return instance;
    }

private:
    static std::shared_ptr<ProcessEventDispatcherImplLinux> instance;
    std::shared_ptr<ProcessWatcher> process_watcher;
    Callback cb;
};
    
std::shared_ptr<IProcessEventDispatcherImplPlatform> getPlatformImpl() {
    return std::reinterpret_pointer_cast<IProcessEventDispatcherImplPlatform>(ProcessEventDispatcherImplLinux::getInstance());
}

#endif