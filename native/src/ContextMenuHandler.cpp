#include "ContextMenuHandler.h"

// Disables the context menu on release builds.
void ContextMenuHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) {
#ifndef _DEBUG
    model->Clear();
#endif
}
