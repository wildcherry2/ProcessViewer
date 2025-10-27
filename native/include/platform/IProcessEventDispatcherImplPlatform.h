#pragma once
#include "../ProcessInfo.h"
#include <functional>
#include <vector>
#include <filesystem>

struct IProcessEventDispatcherImplPlatform {
    using Callback = std::function<void(const ProcessInfoUpdate&)>;

    virtual ~IProcessEventDispatcherImplPlatform() = default;
    virtual void startListening() = 0;
    virtual void stopListening() = 0;
    virtual bool isListening() const = 0;
    virtual bool killProcess(unsigned long pid) const = 0;
    virtual bool systemPrefersDarkMode() const = 0;
    virtual std::vector<uint8_t> getExeIcon(unsigned long pid, const std::wstring& path) const = 0;
    virtual std::vector<ProcessInfoUpdate> getAllProcessesAsAddedEvent() = 0;
    virtual void setCallback(Callback cb) = 0;
};