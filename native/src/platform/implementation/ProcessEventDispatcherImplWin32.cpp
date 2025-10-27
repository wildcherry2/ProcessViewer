#pragma once
#ifdef _WIN32
#define _WINSOCKAPI_
#include "platform/IProcessEventDispatcherImplPlatform.h"
#include <memory>
#include <thread>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <functional>
#include <cstdint>
#include <cstdio>

#include <windows.h>
#include <psapi.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <stdexcept>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shellapi.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Logger.h"
#include "IconCache.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")

constexpr static GUID ProcessProviderGuid = { 0x22FB2CD6, 0x0E7B, 0x422B, { 0xA0, 0xC7, 0x2F, 0xAD, 0x1F, 0xD0, 0xE7, 0x16 } };
constexpr static UINT WM_ADD_HOOK = WM_USER + 1;
constexpr static UINT WM_REMOVE_HOOK = WM_USER + 2;

VOID WINAPI StaticEventRecordCallback(PEVENT_RECORD event_record);
std::shared_ptr<ProcessInfo> InfoFromEvent(PEVENT_RECORD event_record);
VOID CALLBACK StaticOnTitleChange(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
void TitleThreadMain();

struct ProcessEventDispatcherImplWin32 : public IProcessEventDispatcherImplPlatform {
    ProcessEventDispatcherImplWin32() : IProcessEventDispatcherImplPlatform() {
        log_file.LoggerName = const_cast<LPWSTR>(session_name);
        log_file.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
        log_file.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(StaticEventRecordCallback);
        params.Version = ENABLE_TRACE_PARAMETERS_VERSION;

        props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(malloc(props_size));
        ZeroMemory(props, props_size);
        props->Wnode.BufferSize = props_size;
        props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        props->Wnode.ClientContext = 1;
        props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
        props->EnableFlags = /*EVENT_TRACE_FLAG_JOB |*/ EVENT_TRACE_FLAG_PROCESS /*| EVENT_TRACE_FLAG_IMAGE_LOAD*/ /*| EVENT_TRACE_FLAG_PROCESS_COUNTERS*/;
        props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        HANDLE hToken;
        TOKEN_PRIVILEGES tp;
        LUID luid;
        // might not be needed
        OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        auto adjusted = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        CloseHandle(hToken);
    }
    
    ~ProcessEventDispatcherImplWin32() override {
        IProcessEventDispatcherImplPlatform::~IProcessEventDispatcherImplPlatform();
        stopListening();
        free(props);
    }

    std::vector<uint8_t> getExeIcon(unsigned long pid, const std::wstring& path) const override {
        if(path.empty()) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because the passed in file path is empty!");
            return {};
        }

        HICON icon = nullptr;
        if(ExtractIconExW(path.c_str(), 0, &icon, nullptr, 1) == 0 || !icon) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because ExtractIconExW failed!");
            return {};
        }

        ICONINFO icon_info = {};
        if(!GetIconInfo(icon, &icon_info)) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because GetIconInfo failed!");
            DestroyIcon(icon);
            return {};
        }

        BITMAP bitmap = {};
        if(!GetObject(icon_info.hbmColor, sizeof(BITMAP), &bitmap)) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because GetObject failed!");
            DestroyIcon(icon);
            return {};
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = bitmap.bmWidth;
        bmi.bmiHeader.biHeight = -bitmap.bmHeight; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<uint8_t> pixels(bitmap.bmWidth * bitmap.bmHeight * 4);
        HDC hdc = GetDC(NULL);
        if(!hdc) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because GetDC failed!");
            DestroyIcon(icon);
            return {};
        }
        auto getbits_result = GetDIBits(hdc, icon_info.hbmColor, 0, bitmap.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);
        if(!getbits_result || getbits_result == ERROR_INVALID_PARAMETER) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because GetDIBits failed with return value ", getbits_result, "!");
            DestroyIcon(icon);
            return {};
        }
        if(!ReleaseDC(NULL, hdc)) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because ReleaseDC failed!");
            DestroyIcon(icon);
            DeleteObject(icon_info.hbmColor);
            DeleteObject(icon_info.hbmMask);
            return {};
        }

        for (size_t i = 0; i < pixels.size(); i += 4) {
            std::swap(pixels[i], pixels[i + 2]); // swap B <-> R
        }

        std::vector<uint8_t> png;
        if(!stbi_write_png_to_func(
            [](void* ctx, void* data, int size) {
                auto out = reinterpret_cast<std::vector<uint8_t>*>(ctx);
                out->insert(out->end(), (uint8_t*)data, (uint8_t*)data + size); 
            },
            &png,
            bitmap.bmWidth, bitmap.bmHeight, 4, pixels.data(), bitmap.bmWidth * 4
        )) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get exe icon for pid ", pid, " because stbi_write_png_to_func failed!");
            // no need to early out since we have to destroy objects anyways and png is empty
        }

        DestroyIcon(icon);
        DeleteObject(icon_info.hbmColor);
        DeleteObject(icon_info.hbmMask);
        return png;
    }
    
    bool isListening() const override {
        return etw_pump.joinable();
    }
    
    void startListening() override {
        if(etw_pump.joinable()) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Shouldn't be calling startListening twice!");
        }

        if(!cb) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Callback must be set before starting to listen!");
        }
        
        auto status = StartTraceW(&session_handle, session_name, props);
        if(status != ERROR_SUCCESS && status != ERROR_ALREADY_EXISTS) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Failed to start trace session!");
        }

        if(status != ERROR_ALREADY_EXISTS) {
            status = 0; 

            status = EnableTraceEx2(
                session_handle,
                (LPCGUID)&ProcessProviderGuid,
                EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                TRACE_LEVEL_INFORMATION,
                0,
                0,
                0,
                &params
            );

            if (status != ERROR_SUCCESS) {
                throw std::runtime_error("[ProcessEventDispatcherImplWin32] EnableTraceEx2 failed");
            }
        }
        

        trace_handle = OpenTraceW(&log_file);
        if(trace_handle == INVALID_PROCESSTRACE_HANDLE) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Failed to open trace!");
        }
        
        if(!icon_cache) {
            icon_cache = std::make_shared<IconCache>(instance);
        }

        etw_pump = std::thread([this] () {
            ProcessTrace(&trace_handle, 1, 0, 0);
        });

        title_msg_pump = std::thread(TitleThreadMain);
        dark_mode_watcher = std::jthread(dark_mode_callback);
    }

    void stopListening() override {
        CloseTrace(trace_handle);
        ControlTraceW(session_handle, session_name, props, EVENT_TRACE_CONTROL_STOP);
        if(etw_pump.joinable()) {
            etw_pump.join();
        }
        if(title_msg_pump.joinable()) {
            PostThreadMessage(GetThreadId(title_msg_pump.native_handle()), WM_QUIT, NULL, NULL);
            title_msg_pump.join();
        }

        if(dark_mode_watcher.request_stop()) {
            dark_mode_watcher.join();
        }
        trace_handle = session_handle = NULL;
    }

    void setCallback(Callback cb) override {
        this->cb = cb;
    }

    bool doesProcessExist(unsigned long pid) const {
        auto handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if(handle == INVALID_HANDLE_VALUE) return false;
        CloseHandle(handle);
        return true;
    }

    bool killProcess(unsigned long pid) const override {
        auto handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if(handle == INVALID_HANDLE_VALUE) return false;
        auto result = TerminateProcess(handle, 1);
        CloseHandle(handle);
        return result;
    }

    std::filesystem::path getExeFilePath(unsigned long pid) const {
        HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (handle == INVALID_HANDLE_VALUE) return L"";

        wchar_t buf[MAX_PATH];
        DWORD size = MAX_PATH;
        if (!QueryFullProcessImageNameW(handle, 0, buf, &size)) {
            CloseHandle(handle);
            return L"";
        }

        CloseHandle(handle);
        return std::filesystem::path(std::wstring(buf, size));
    }

    std::vector<ProcessInfoUpdate> getAllProcessesAsAddedEvent() override {
        std::vector<ProcessInfoUpdate> result;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Failed to create process snapshot.");
        }
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe)) {
            do {
                auto pi = std::make_shared<ProcessInfo>(pe.szExeFile, getExeFilePath(pe.th32ProcessID), pe.th32ProcessID);
                setupProcessInfoTitleAndIcon(pi);
                result.emplace_back(EProcessInfoEvent::PROCESS_ADDED, pi); // todo add names
            } while (Process32Next(hSnapshot, &pe));
        } else {
            CloseHandle(hSnapshot);
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Failed to get first process.");
        }
        CloseHandle(hSnapshot);
        return result;
    }

    std::shared_ptr<std::wstring> getWindowTitleFromProcess(unsigned long pid) {
        struct EnumData {
            DWORD pid;
            HWND hwnd = NULL;
        };
        EnumData data{pid};
        EnumWindows([](HWND hwnd, LPARAM p_data){
            auto* data = reinterpret_cast<EnumData*>(p_data);
            DWORD this_pid = 0;
            GetWindowThreadProcessId(hwnd, &this_pid);
            if(data->pid != this_pid) return TRUE;
            if (GetAncestor(hwnd, GA_ROOT) == hwnd) {
                auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
                if (!(style & WS_EX_TOOLWINDOW)) {
                    data->hwnd = hwnd;
                    return FALSE; // stop enumeration
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&data));
        if(!data.hwnd) return nullptr;
        auto length = GetWindowTextLengthW(data.hwnd);
        if(!length) return nullptr;
        auto ret_ptr = std::make_shared<std::wstring>(length + 1, '\0');
        if(!GetWindowTextW(data.hwnd, ret_ptr->data(), length + 1)) return nullptr;
        ret_ptr->resize(length);
        _RDEBUGW(L"[ProcessEventDispatcherImplWin32] Set title for ", pid, " ", *ret_ptr);
        return ret_ptr;
    }

    static std::shared_ptr<ProcessEventDispatcherImplWin32> getInstance() {
        if(!instance) {
            instance = std::make_shared<ProcessEventDispatcherImplWin32>();
        }
        return instance;
    }

private:
    static std::shared_ptr<ProcessEventDispatcherImplWin32> instance;

    // note target data is stored in event_record->UserData
    // logman stop "InjProcessLogger" -ets
    void eventRecordCallback(PEVENT_RECORD event_record) {
        auto type = event_record->EventHeader.EventDescriptor.Id;
        if(type != 1 && type != 2) return;
        try {
            ProcessInfoUpdate update{ type == 1 ? EProcessInfoEvent::PROCESS_ADDED : EProcessInfoEvent::PROCESS_REMOVED };
            auto data = InfoFromEvent(event_record);
            if(!data) {
                _RWARNW(L"[ProcessEventDispatcherImplWin32] Error processing process event -- couldn't null info from event!");
                return;
            }
            if(type == 1) {
                if(!doesProcessExist(data->pid)) {
                    _RDEBUGW(L"Received out of order create before destroy events for process ", data->pid);
                    return;
                }
                setupProcessInfoTitleAndIcon(data);
                update.data = std::reinterpret_pointer_cast<void>(data);
            }
            else {
                update.data = std::reinterpret_pointer_cast<void>(std::make_shared<unsigned long>(static_cast<unsigned long>(data->pid)));
                PostThreadMessage(GetThreadId(title_msg_pump.native_handle()), WM_REMOVE_HOOK, static_cast<WPARAM>(data->pid), NULL);
            }
            cb(update);

        }
        catch(const std::exception& e) {
            _RERR( "[ProcessEventDispatcherImplWin32] Error processing process event -- ", e.what());
        }
    }

    void onTitleChange(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime) {
        if (idObject != OBJID_WINDOW || !hwnd) return;
        DWORD pid = 0;
        if(!GetWindowThreadProcessId(hwnd, &pid)) {
            _RERR("[ProcessEventDispatcherImplWin32] Failed to get pid for hwnd ", hwnd);
            return;
        }
        _RDEBUG("[ProcessEventDispatcherImplWin32] Window title change detected, pid = ", pid);
        wchar_t title[512];
        int len = GetWindowTextW(hwnd, title, 512);
        if(!len) {
            _RDEBUG("[ProcessEventDispatcherImplWin32] pid ", pid, " doesn't have window text!");
            return;
        }
        ProcessInfoUpdate update = { 
            EProcessInfoEvent::PROCESS_TITLEUPDATE, 
            std::reinterpret_pointer_cast<void>(std::make_shared<ProcessTitleUpdateData>(std::make_shared<std::wstring>(title, len), pid, dwmsEventTime)) 
        };
        cb(update);
    }

    std::function<void(std::stop_token)> dark_mode_callback = [this](std::stop_token token) -> void {
        HKEY dark_mode_key;
        bool is_dark_mode = false;
        bool needs_registration = true;
        bool initialized = false;

        if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_NOTIFY | KEY_READ, &dark_mode_key) != ERROR_SUCCESS) {
            _RWARN("[ProcessEventDispatcherImplWin32] Dark mode registry key not found, assuming light mode only!");
            return;
        }

        auto event_handle = CreateEventW(nullptr, TRUE, TRUE, nullptr);
        if(event_handle == NULL) {
            _RWARN("[ProcessEventDispatcherImplWin32] Could not create dark mode event handle, assuming light mode only!");
            RegCloseKey(dark_mode_key);
            return;
        }

        while(!token.stop_requested()) {
            if(needs_registration && RegNotifyChangeKeyValue(dark_mode_key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, event_handle, TRUE) != ERROR_SUCCESS) {
                _RWARN("[ProcessEventDispatcherImplWin32] Dark mode can't be watched, assuming current mode only!");
                break;
            }
            needs_registration = false;
            auto wait_result = WaitForSingleObject(event_handle, 1000);
            if(wait_result == WAIT_OBJECT_0 || !initialized) {
                DWORD light_mode_value = 0;
                static DWORD size = sizeof(DWORD);
                if(auto result = RegQueryValueExW(dark_mode_key, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&light_mode_value), &size); result != ERROR_SUCCESS) {
                    _RWARN("[ProcessEventDispatcherImplWin32] Failed to query AppsUseLightTheme, assuming current mode only! Result = ", result);
                    break;
                }
                ResetEvent(event_handle);
                bool iteration_is_dark_mode = light_mode_value == 0;
                needs_registration = true;
                initialized = true;
                if(iteration_is_dark_mode == is_dark_mode) continue;
                is_dark_mode = iteration_is_dark_mode;
                _RDEBUG("[ProcessEventDispatcherImplWin32] Dark mode set to ", is_dark_mode);
                cb({EProcessInfoEvent::PROCESS_DARKMODECHANGE, std::reinterpret_pointer_cast<void>(std::make_shared<bool>(is_dark_mode))});
            }
        }

        CloseHandle(event_handle);
        RegCloseKey(dark_mode_key);
    };

    void setupProcessInfoTitleAndIcon(std::shared_ptr<ProcessInfo> info) {
        icon_cache->setIconInProcess(info);
        auto id = GetThreadId(title_msg_pump.native_handle());
        PostThreadMessage(id, WM_ADD_HOOK, static_cast<WPARAM>(info->pid), NULL); // TODO figure out a way to await the hook, just to avoid the very rare edge case of missing a title update
        info->title = getWindowTitleFromProcess(info->pid); // TODO refactor to set the top-level-window field, though it's not urgent
    }

    EVENT_TRACE_LOGFILEW log_file = { 0 };
    ENABLE_TRACE_PARAMETERS params = { 0 };
    TRACEHANDLE trace_handle = NULL;
    TRACEHANDLE session_handle = NULL;
    std::thread etw_pump;
    std::thread title_msg_pump;
    std::jthread dark_mode_watcher;
    std::shared_ptr<IconCache> icon_cache;

    constexpr static size_t props_size = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(WCHAR);
    EVENT_TRACE_PROPERTIES* props = nullptr;
    constexpr static const wchar_t* session_name = L"ProcessEventDispatcherLogger";

    Callback cb;

    friend VOID WINAPI StaticEventRecordCallback(PEVENT_RECORD event_record);
    friend VOID CALLBACK StaticOnTitleChange(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
};

std::shared_ptr<ProcessEventDispatcherImplWin32> ProcessEventDispatcherImplWin32::instance = nullptr;

static VOID WINAPI StaticEventRecordCallback(PEVENT_RECORD event_record) {
    return ProcessEventDispatcherImplWin32::getInstance()->eventRecordCallback(event_record);
}

static VOID CALLBACK StaticOnTitleChange(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime) {
    return ProcessEventDispatcherImplWin32::getInstance()->onTitleChange(hWinEventHook, event, hwnd, idObject, idChild, idEventThread, dwmsEventTime);
}

static std::shared_ptr<ProcessInfo> InfoFromEvent(PEVENT_RECORD event_record) {
    static thread_local ULONG actual_buffer_size = 0;
    static thread_local ULONG working_buffer_size = 0;
    static thread_local PTRACE_EVENT_INFO info = nullptr;
    constexpr static size_t SENTINEL = UINT64_MAX;

    struct PropIdx { size_t pid = SENTINEL; size_t file_name = SENTINEL; };
    struct PropKey { 
        const GUID provider; 
        const USHORT id; 
        const UCHAR version;

        std::strong_ordering operator<=>(const PropKey& other) const noexcept {
            if (auto c = id <=> other.id; c != 0) return c;
            if (auto c = version <=> other.version; c != 0) return c;
            int cmp = std::memcmp(&provider, &other.provider, sizeof(GUID));
            if (cmp < 0) return std::strong_ordering::less;
            if (cmp > 0) return std::strong_ordering::greater;
            return std::strong_ordering::equal;
        }
    };
    static thread_local std::map<PropKey, PropIdx> prop_map;

    if(info == nullptr) {
        TdhGetEventInformation(event_record, 0, nullptr, nullptr, &working_buffer_size);
        info = reinterpret_cast<PTRACE_EVENT_INFO>(malloc(working_buffer_size));
        actual_buffer_size = working_buffer_size;

        auto status = TdhGetEventInformation(event_record, 0, nullptr, info, &working_buffer_size);
        if(status != ERROR_SUCCESS) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] TdhGetEventInformation failed with status " + std::to_string(status));
        }
    }
    else {
        //ZeroMemory(info, actual_buffer_size);
        auto status = TdhGetEventInformation(event_record, 0, nullptr, info, &working_buffer_size);
        if(status == ERROR_INSUFFICIENT_BUFFER) {
            free(info);
            info = reinterpret_cast<PTRACE_EVENT_INFO>(malloc(working_buffer_size));
            actual_buffer_size = working_buffer_size;
            status = TdhGetEventInformation(event_record, 0, nullptr, info, &working_buffer_size);
            if(status != ERROR_SUCCESS) {
                throw std::runtime_error("[ProcessEventDispatcherImplWin32] TdhGetEventInformation failed with status " + std::to_string(status));
            }
        }
        else if(status != ERROR_SUCCESS) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] TdhGetEventInformation failed with status " + std::to_string(status));
        }
    }

    const auto& header = event_record->EventHeader;
    const auto& desc = header.EventDescriptor;
    const auto key = PropKey{ header.ProviderId, desc.Id, desc.Version };
    auto idx_it = prop_map.find(key);
    if(idx_it == prop_map.end()) {
        PropIdx idx{};
        for(ULONG i = 0; i < info->TopLevelPropertyCount; ++i) {
            auto& prop = info->EventPropertyInfoArray[i];
            LPCWSTR prop_name = (LPCWSTR)((BYTE*)info + prop.NameOffset);
            if((wcscmp(prop_name, L"ProcessID") == 0) || (wcscmp(prop_name, L"ProcessId") == 0)) {
                idx.pid = i;
                if(idx.pid != SENTINEL && idx.file_name != SENTINEL) {
                    break;
                }
                continue;
            }
            if((wcscmp(prop_name, L"ImageName") == 0) || (wcscmp(prop_name, L"ImageFileName") == 0)) {
                idx.file_name = i;
                if(idx.pid != SENTINEL && idx.file_name != SENTINEL) {
                    break;
                }
                continue;
            }
        }
        if(idx.pid == SENTINEL) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] Couldn't find ProcessId property in event!");
        }

        idx_it = prop_map.insert({ key, idx }).first;
    }

    auto result = std::make_shared<ProcessInfo>();

    {
        PROPERTY_DATA_DESCRIPTOR desc{};
        desc.PropertyName = (ULONGLONG)((PBYTE)info + info->EventPropertyInfoArray[idx_it->second.pid].NameOffset);
        desc.ArrayIndex = ULONG_MAX;

        auto status = TdhGetProperty(event_record, 0, nullptr, 1, &desc, sizeof(unsigned long), reinterpret_cast<PBYTE>(&result->pid));
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("[ProcessEventDispatcherImplWin32] TdhGetProperty for ProcessId failed with status " + std::to_string(status));
        }
    }
    
    if(idx_it->second.file_name != SENTINEL) {
        auto instance = ProcessEventDispatcherImplWin32::getInstance(); 
        auto full_path = instance->getExeFilePath(result->pid);
        if(full_path.empty()) {
            //if(!instance->doesProcessExist(result->pid)) return nullptr;
            result->exe_name = L"Unknown Process";
        }
        else {
            result->exe_name = full_path.filename().wstring();
            result->exe_path = full_path.wstring();
        }
    }
    
    return result;
}

static void TitleThreadMain() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        static thread_local std::map<unsigned long, HWINEVENTHOOK> hook_map;
        switch(msg.message) {
            case WM_ADD_HOOK: {
                if(hook_map.contains(static_cast<DWORD>(msg.wParam))) {
                    _RWARN("Tried to hook an already hooked process (pid = ", static_cast<DWORD>(msg.wParam), ")");
                    break;
                }
                hook_map[static_cast<DWORD>(msg.wParam)] = SetWinEventHook( // needs to be moved off thread
                                                            EVENT_OBJECT_NAMECHANGE,
                                                            EVENT_OBJECT_NAMECHANGE,
                                                            NULL,
                                                            StaticOnTitleChange,
                                                            static_cast<DWORD>(msg.wParam),
                                                            0,       // 0 = all threads
                                                            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
                                                        );
                break;
            }
            case WM_REMOVE_HOOK: {
                auto hook = hook_map.extract(static_cast<DWORD>(msg.wParam));
                if(!hook.empty()) {
                    UnhookWinEvent(hook.mapped());
                }
                else {
                    _RWARN("Tried to remove a hook that doesn't exist!");
                }
                break;
            }
            case WM_QUIT: {
                for(auto& hook : hook_map) {
                    UnhookWinEvent(hook.second);
                }
                hook_map.clear();
                break;
            }
            default: {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}

std::shared_ptr<IProcessEventDispatcherImplPlatform> getPlatformImpl() {
    return std::reinterpret_pointer_cast<IProcessEventDispatcherImplPlatform>(ProcessEventDispatcherImplWin32::getInstance());
}

#endif