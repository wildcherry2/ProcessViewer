#include "NetworkResourceRequestHandler.h"
#include <string_view>
#include "Defines.h"
#include "Logger.h"

cef_return_value_t NetworkRequestHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, 
                                                               CefRefPtr<CefFrame> frame, 
                                                               CefRefPtr<CefRequest> request, 
                                                               CefRefPtr<CefCallback> callback) {
    auto url = request->GetURL().ToWString();
    if(!url.starts_with(SCHEMA) && !url.starts_with(L"devtools")) { // will probably have to redirect file:// 3
        _RERR("[NetworkRequestHandler] Blocked non-injapp request: ", url);
        return RV_CANCEL; //block non injapp requests
    }
    return RV_CONTINUE;
}
