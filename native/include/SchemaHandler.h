#pragma once
#include <string>
#include <memory>
#include "include/cef_resource_handler.h"

class SchemaHandler : public CefResourceHandler {
public:
    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) override;
    bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) override;
    void Cancel() override;
private:
    IMPLEMENT_REFCOUNTING(SchemaHandler);

    struct State;
    std::shared_ptr<State> state = nullptr;
};