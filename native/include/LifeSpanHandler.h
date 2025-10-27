#pragma once
#include "include/cef_life_span_handler.h"

class LifeSpanHandler : public CefLifeSpanHandler {
public:
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    ~LifeSpanHandler() override = default;
private:
    IMPLEMENT_REFCOUNTING(LifeSpanHandler);
    size_t browser_count = 0;
};