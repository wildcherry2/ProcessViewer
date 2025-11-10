#include "NetworkResourceRequestHandler.h"
#include <string_view>
#include "Defines.h"
#include "Logger.h"

// Filters out rogue HTTP requests, preventing any 'phone-home' attempts from potentially malicious libraries since this uses an unsandboxed renderer.
cef_return_value_t NetworkRequestHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, 
                                                               CefRefPtr<CefFrame> frame, 
                                                               CefRefPtr<CefRequest> request, 
                                                               CefRefPtr<CefCallback> callback) {
    auto url = request->GetURL().ToWString();
    if(!url.starts_with(SCHEMA) && !url.starts_with(L"devtools")) {
        _RERR("[NetworkRequestHandler] Blocked non-schema request: ", url);
        return RV_CANCEL; //block non schema/devtool requests
    }
    return RV_CONTINUE;
}
