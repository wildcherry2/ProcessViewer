#pragma once
#include "include/cef_client.h"

class PMClient : public CefClient {
public:
    CefRefPtr<CefRequestHandler> GetRequestHandler() override;
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
private:
    IMPLEMENT_REFCOUNTING(PMClient);
};