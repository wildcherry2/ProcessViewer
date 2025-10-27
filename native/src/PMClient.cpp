#include "PMClient.h"
#include "RequestHandler.h"
#include "ConsoleHandler.h"
#include "LifeSpanHandler.h"

CefRefPtr<CefRequestHandler> PMClient::GetRequestHandler() {
    return new RequestHandler();
}

CefRefPtr<CefDisplayHandler> PMClient::GetDisplayHandler() {
    return new ConsoleHandler();
}

CefRefPtr<CefLifeSpanHandler> PMClient::GetLifeSpanHandler() {
    return new LifeSpanHandler();
}
