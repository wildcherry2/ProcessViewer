#include "ContextMenuHandler.h"

void ContextMenuHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) {
#ifndef _DEBUG
    model->Clear();
#endif
}
