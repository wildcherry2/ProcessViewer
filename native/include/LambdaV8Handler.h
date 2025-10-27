#pragma once
#include <functional>
#include "include/cef_v8.h"

class LambdaV8Handler : public CefV8Handler {
public:
    using ExecuteCallback = std::function<bool(const CefString&, CefRefPtr<CefV8Value>, const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&)>;

    LambdaV8Handler(ExecuteCallback cb);
    LambdaV8Handler() = delete;
    ~LambdaV8Handler() override = default;

    bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) override;
private:
    IMPLEMENT_REFCOUNTING(LambdaV8Handler);

    ExecuteCallback cb;
};