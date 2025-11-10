#include "LifeSpanHandler.h"
#include <include/cef_app.h>
#include <include/wrapper/cef_helpers.h>

// Counts the spawned browsers after they're created (at most 2 in debug mode with dev tools).
void LifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    ++browser_count;
}

// Shuts down CEF when browsers are closed.
void LifeSpanHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if(--browser_count == 0) 
        CefQuitMessageLoop(); 
}
