#include "LambdaV8Handler.h"

LambdaV8Handler::LambdaV8Handler(ExecuteCallback cb): CefV8Handler(), cb(cb) {}

bool LambdaV8Handler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) {
    return cb(name, object, arguments, retval, exception);
}
