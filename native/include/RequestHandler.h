#pragma once
#include "include/cef_request_handler.h"

class RequestHandler : public CefRequestHandler {
public:
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool is_navigation,
        bool is_download,
        const CefString& request_initiator,
        bool& disable_default_handling) override;
    ~RequestHandler() override = default;
private:
    IMPLEMENT_REFCOUNTING(RequestHandler);
};