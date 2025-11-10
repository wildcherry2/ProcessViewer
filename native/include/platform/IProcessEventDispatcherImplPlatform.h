#pragma once
#include "../ProcessInfo.h"
#include <functional>
#include <vector>
#include <filesystem>

struct IProcessEventDispatcherImplPlatform {
    using Callback = std::function<void(const ProcessInfoUpdate&)>;

    virtual ~IProcessEventDispatcherImplPlatform() = default;

    // Start listening for updates
    virtual void startListening() = 0;

    // Stop listening for updates
    virtual void stopListening() = 0;

    // Returns true if listening for updates
    virtual bool isListening() const = 0;

    // Attempts to kill the process with the given PID
    virtual bool killProcess(unsigned long pid) const = 0;

    // Returns true if the underlying platform has dark mode enabled
    virtual bool systemPrefersDarkMode() const = 0;

    // Returns a vector of bytes that represents an executable's icon as a PNG
    virtual std::vector<uint8_t> getExeIcon(unsigned long pid, const std::wstring& path) const = 0;

    // Returns current list of processes as a PROCESS_ADDED update.
    virtual std::vector<ProcessInfoUpdate> getAllProcessesAsAddedEvent() = 0;

    // Sets the callback that can be used to enqueue an update to the UI
    virtual void setCallback(Callback cb) = 0;
};