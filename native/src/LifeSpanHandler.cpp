#include "LifeSpanHandler.h"
#include <include/cef_app.h>
#include <include/wrapper/cef_helpers.h>

void LifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    ++browser_count;
}

void LifeSpanHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if(--browser_count == 0) 
        CefQuitMessageLoop(); 
}
