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

void PMApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    ProcessController::getInstance()->startMonitoring(context); // need to discriminate between different browser instances (dev tools, popups, etc)
}

void PMApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    registrar->AddCustomScheme(SCHEMA, CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_LOCAL | CEF_SCHEME_OPTION_SECURE | CEF_SCHEME_OPTION_FETCH_ENABLED);
}

void PMApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_RENDERER_THREAD();
    ProcessController::getInstance()->stopMonitoring();
}

void PMApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)  {
        // Enable GPU acceleration
        /*command_line->AppendSwitch("enable-gpu");
        command_line->AppendSwitch("enable-gpu-compositing");*/
        //command_line->AppendSwitch("--show-fps-counter");
        // Optional: force WebGL or accelerated 2D canvas
        //command_line->AppendSwitch("ignore-gpu-blacklist");
    }

CefRefPtr<CefLoadHandler> PMApp::GetLoadHandler() {
    return this;
}

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
