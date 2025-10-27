#pragma once
#include "include/cef_client.h"

class PMClient : public CefClient {
public:
    PMClient();
    CefRefPtr<CefRequestHandler> GetRequestHandler() override;
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
private:
    IMPLEMENT_REFCOUNTING(PMClient);

    CefRefPtr<CefRequestHandler> request_handler;
    CefRefPtr<CefDisplayHandler> display_handler;
    CefRefPtr<CefLifeSpanHandler> lifespan_handler;
    CefRefPtr<CefContextMenuHandler> ctxmenu_handler;
};