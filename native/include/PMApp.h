#pragma once
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_base.h"
#include "include/wrapper/cef_helpers.h"

#include "PMClient.h"

class PMApp : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler, public CefLoadHandler {
public:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;
    void OnContextInitialized() override;
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;

    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;
    void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> command_line) override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override;
private:
    IMPLEMENT_REFCOUNTING(PMApp);
};