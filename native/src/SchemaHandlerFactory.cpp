#include "SchemaHandlerFactory.h"
#include "SchemaHandler.h"

CefRefPtr<CefResourceHandler> SchemaHandlerFactory::Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request) {
    return new SchemaHandler();
}
