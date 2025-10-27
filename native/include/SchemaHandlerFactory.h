#pragma once
#include "include/cef_scheme.h"

class SchemaHandlerFactory : public CefSchemeHandlerFactory {
public:
    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         const CefString& scheme_name,
                                         CefRefPtr<CefRequest> request) override;

private:
    IMPLEMENT_REFCOUNTING(SchemaHandlerFactory);
};