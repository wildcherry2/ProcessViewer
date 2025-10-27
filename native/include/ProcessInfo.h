#pragma once
#include <string>
#include <memory>
#include "include/internal/cef_string.h"

struct ProcessInfo {
    std::wstring exe_name;                          // [PLATFORM-SUPPLIED] Name of executable file
    std::wstring exe_path;                          // [PLATFORM-SUPPLIED] Path of executable file
    unsigned long pid = 0;                          // [PLATFORM-SUPPLIED] PID of process
    bool hidden = false;                            // Whether or not this process is currently filtered out in the ui
    bool is_top_level_window = false;               // [PLATFORM-SUPPLIED] Whether this process is a top-level window
    std::shared_ptr<const CefString> exe_icon_b64;  // [PLATFORM-SUPPLIED] A pointer to the base64 encoded PNG icon of the process, to be set in a <img> src attribute, if it has one
    std::shared_ptr<std::wstring> title;            // [PLATFORM-SUPPLIED] A pointer to the title of the window, iff the process is_top_level_window
};

enum EProcessInfoEvent {
    PROCESS_ADDED,          // [PLATFORM-SUPPLED] data is std::shared_ptr<ProcessInfo>, use std::reinterpret_pointer_cast<ProcessInfo>(data)
    PROCESS_REMOVED,        // [PLATFORM-SUPPLED] data is PID, std::shared_ptr<unsigned long> use std::reinterpret_pointer_cast<unsigned long>(data)
    PROCESS_HIDDEN,         // [SEARCH_ENGINE] data is PID, std::shared_ptr<std::vector<unsigned long>> use std::reinterpret_pointer_cast<unsigned long>(data)
    PROCESS_UNHIDDEN,       // [SEARCH_ENGINE] data is PID, std::shared_ptr<std::vector<unsigned long>> use std::reinterpret_pointer_cast<unsigned long>(data),
    PROCESS_SHOWALL,        // [SEARCH_ENGINE] data is nullptr/indeterminate
    PROCESS_HIDEALL,        // [SEARCH_ENGINE] data is nullptr/indeterminate
    PROCESS_TITLEUPDATE,    // [PLATFORM-SUPPLED] data is std::shared_ptr<ProcessTitleUpdateData>, use std::reinterpret_pointer_cast<ProcessTitleUpdateData>(data)
    PROCESS_DARKMODECHANGE, // [PLATFORM-SUPPLED] data is bool, use std::reinterpret_pointer_cast<bool>(data); if true, dark mode should be enabled, otherwise light mode should be enabled
    UNKNOWN,                // data is void*/indeterminate
    CONTROL_TERMINATE,      // data is std::shared_ptr<ProcessInfo>, use std::reinterpret_pointer_cast<ProcessInfo>(data)
    CONTROL_UPDATE_SEARCH   // data is std::shared_ptr<std::wstring>, use std::reinterpret_pointer_cast<std::wstring>(data)
};

struct ProcessInfoUpdate {
    EProcessInfoEvent event = EProcessInfoEvent::UNKNOWN;
    std::shared_ptr<void> data = nullptr;
};

struct ProcessTitleUpdateData {
    std::shared_ptr<std::wstring> title;
    unsigned long pid;
    unsigned long time;
};