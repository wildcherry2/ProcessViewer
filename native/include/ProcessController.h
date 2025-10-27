#pragma once
#include <memory>
#include "include/cef_v8.h"

class ProcessController {
public:
    void startMonitoring(CefRefPtr<CefV8Context> context);
    void stopMonitoring();
    bool systemPrefersDarkMode();
    ~ProcessController();
    static std::shared_ptr<ProcessController> getInstance();
private:

    ProcessController();
    static std::shared_ptr<ProcessController> instance;
    
    struct Impl;
    std::unique_ptr<Impl> pimpl; // pimpl for hiding cross platform implementation details
};