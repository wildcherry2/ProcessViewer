#pragma once
#include "include/cef_resource_request_handler.h"

class NetworkRequestHandler : public CefResourceRequestHandler {
public:
    cef_return_value_t OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request,
                                           CefRefPtr<CefCallback> callback) override;
    ~NetworkRequestHandler() override = default;
private:
    IMPLEMENT_REFCOUNTING(NetworkRequestHandler);
};