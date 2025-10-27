#pragma once
#include "include/cef_context_menu_handler.h"

class ContextMenuHandler : public CefContextMenuHandler {
public:
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefContextMenuParams> params,
                                   CefRefPtr<CefMenuModel> model) override;
    ~ContextMenuHandler() override = default;

private:
    IMPLEMENT_REFCOUNTING(ContextMenuHandler);
};