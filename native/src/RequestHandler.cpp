#include "RequestHandler.h"
#include "NetworkResourceRequestHandler.h"

CefRefPtr<CefResourceRequestHandler> RequestHandler::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling) {
    return new NetworkRequestHandler();
}
