#include "PMApp.h"
#include <filesystem>
#include "Defines.h"
#include "SchemaHandlerFactory.h"
#include "ProcessController.h"
#include "Logger.h"

CefRefPtr<CefBrowserProcessHandler> PMApp::GetBrowserProcessHandler() {
    return this;
}

CefRefPtr<CefRenderProcessHandler> PMApp::GetRenderProcessHandler() {
    return this;
}

// (UI Thread) Initializes custom schema handler and creates the browser.
void PMApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    CefBrowserSettings browser_settings;
    CefWindowInfo window_info;
    window_info.SetAsPopup(NULL, WINDOW_NAME);
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    auto html = std::wstring(SCHEMA) + L"://index.html";
    CefRegisterSchemeHandlerFactory(SCHEMA, "", new SchemaHandlerFactory());
    CefBrowserHost::CreateBrowser(window_info, new PMClient(), html, browser_settings, nullptr, nullptr);
}

// (Renderer Thread) Initializes the ProcessController singleton.
void PMApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    CEF_REQUIRE_RENDERER_THREAD();
    ProcessController::getInstance()->startMonitoring(context);
}

// Informs the passed in registrar about the custom schema.
void PMApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    registrar->AddCustomScheme(SCHEMA, CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_LOCAL | CEF_SCHEME_OPTION_SECURE | CEF_SCHEME_OPTION_FETCH_ENABLED);
}

// (Renderer Thread) Stops monitoring for process changes when the browser is destroyed.
void PMApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_RENDERER_THREAD();
    ProcessController::getInstance()->stopMonitoring();
}

CefRefPtr<CefLoadHandler> PMApp::GetLoadHandler() {
    return this;
}

// (Renderer Thread) Once the document is loaded, sets up dark mode if the system has it enabled (media queries are disabled in CEF).
void PMApp::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    CEF_REQUIRE_RENDERER_THREAD();
    if(!frame->IsMain()) return;
    auto ctx = frame->GetV8Context();
    if(!ctx->IsValid()) {
        _RERR("Couldn't set dark mode after load end!");
        return;
    }

    if(!ProcessController::getInstance()->systemPrefersDarkMode()) return;

    if(!ctx->Enter()) {
        _RERR("Failed to enter context in load end!");
        return;
    }
    auto global = ctx->GetGlobal();
    if(!global->IsValid()) {
        _RERR("Couldn't get global object in load end!");
        ctx->Exit();
        return;
    }
    auto html = global->GetValue("document")->GetValue("documentElement");
    auto setAttribute = html->GetValue("setAttribute");
    setAttribute->ExecuteFunction(html, { CefV8Value::CreateString("dark-mode"), CefV8Value::CreateString("") });

    ctx->Exit();
}
