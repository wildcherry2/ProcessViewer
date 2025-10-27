#pragma once
#include <string>
#include <map>
#include <memory>
#include "include/cef_parser.h"

struct IProcessEventDispatcherImplPlatform;
struct ProcessInfo;

class IconCache {
public:
    IconCache() = delete; 
    IconCache(std::shared_ptr<IProcessEventDispatcherImplPlatform> platform);

    void setIconInProcess(std::shared_ptr<ProcessInfo> process);
private:
    using PNGData = const CefString;
    std::map<std::wstring, std::shared_ptr<PNGData>> cache;
    std::shared_ptr<IProcessEventDispatcherImplPlatform> platform;
};