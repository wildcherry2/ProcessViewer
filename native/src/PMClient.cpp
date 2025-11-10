#include "PMClient.h"
#include "RequestHandler.h"
#include "ConsoleHandler.h"
#include "LifeSpanHandler.h"
#include "ContextMenuHandler.h"

PMClient::PMClient() : CefClient(), request_handler(new RequestHandler()), display_handler(new ConsoleHandler()), lifespan_handler(new LifeSpanHandler()), ctxmenu_handler(new ContextMenuHandler()) {}

CefRefPtr<CefRequestHandler> PMClient::GetRequestHandler() {
    return request_handler;
}

// 'display_handler' is the ConsoleHandler
CefRefPtr<CefDisplayHandler> PMClient::GetDisplayHandler() {
    return display_handler;
}

CefRefPtr<CefLifeSpanHandler> PMClient::GetLifeSpanHandler() {
    return lifespan_handler;
}

CefRefPtr<CefContextMenuHandler> PMClient::GetContextMenuHandler() {
    return ctxmenu_handler;
}
