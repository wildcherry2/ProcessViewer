#include "PMApp.h"
#include <filesystem>
#include "Defines.h"
#include "SchemaHandlerFactory.h"
#include "ProcessController.h"

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
