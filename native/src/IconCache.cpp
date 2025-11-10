#include "IconCache.h"

#include "ProcessInfo.h"
#include "platform/IProcessEventDispatcherImplPlatform.h"
#include "Logger.h"

IconCache::IconCache(std::shared_ptr<IProcessEventDispatcherImplPlatform> platform) : platform(platform) {}

// Acts as a cache for executable icons. Uses IProcessEventDispatcherImplPlatform::getExeIcon and encodes it in base64 when the cache misses.
void IconCache::setIconInProcess(std::shared_ptr<ProcessInfo> process) {
    auto& path = process->exe_path;
    if(path.empty()) {
        _RERRW(L"[IconCache] Can't get the icon for ", process->pid, " ", process->exe_name, " because it doesn't have a path!");
        return;
    }
    auto cached = cache.find(path);
    if(cached != cache.end()) {
        process->exe_icon_b64 = cached->second;
        return;
    }
    auto png_bytes = platform->getExeIcon(process->pid, path);
    if(png_bytes.empty()) return;
    auto url = std::make_shared<IconCache::PNGData>("data:image/png;base64," + CefBase64Encode(png_bytes.data(), png_bytes.size()).ToString());
    cache[path] = url;
    process->exe_icon_b64 = url;
    _RDEBUGW(L"Created icon for ", process->exe_name);
    return;
}
