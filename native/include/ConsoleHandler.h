#pragma once
#include "include/cef_display_handler.h"

class ConsoleHandler : public CefDisplayHandler {
public:
    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                cef_log_severity_t level,
                                const CefString& message,
                                const CefString& source,
                                int line) override;

    ~ConsoleHandler() override = default;
private:
    IMPLEMENT_REFCOUNTING(ConsoleHandler);
};