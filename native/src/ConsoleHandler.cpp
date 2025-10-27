#include "ConsoleHandler.h"
#include "Logger.h"

bool ConsoleHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) {
    switch(level) {
        case LOGSEVERITY_DEFAULT:
        case LOGSEVERITY_INFO: {
            _RLOGW(message.ToWString());
            break;
        }
        case LOGSEVERITY_WARNING: {
            _RWARNW(message.ToWString(), L" (", source.ToWString(), L", line ", line, L")");
            break;
        }
        case LOGSEVERITY_DEBUG: {
            _RDEBUGW(message.ToWString(), L" (", source.ToWString(), L", line ", line, L")");
            break;
        }
        default: {
            _RERRW(message.ToWString(), L" (", source.ToWString(), L", line ", line, L")");
        }
    }
    return true;
}
