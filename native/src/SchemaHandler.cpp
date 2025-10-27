#include "SchemaHandler.h"
#include <set>
#include <filesystem>
#include <fstream>

#include "Defines.h"
#include "Logger.h"

struct SchemaHandler::State {
    bool error = false;
    size_t offset = 0;
    std::string data;
    std::string mime;
};

bool SchemaHandler::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    static std::set<std::wstring> allowed_files = { JSFILES };
    state = std::make_shared<State>();

    auto req = request->GetURL().ToWString();
    if(req[req.size() - 1] == '/') req.pop_back();
    auto file_idx = req.find_last_of('/');

    _RDEBUGW(L"[SchemaHandler] Requested resource: ", req);

    if(file_idx == std::wstring::npos || file_idx + 1 == req.size()) {
        _RERRW(L"[SchemaHandler] Malformed request URL (missing file name): ", req);
        state->error = true;
        callback->Continue();
        return true;
    }

    auto file = req.substr(file_idx + 1);
    if(allowed_files.find(file) == allowed_files.end()) {
        _RERRW(L"[SchemaHandler] Requested file is not allowed: ", file);
        state->error = true;
        callback->Continue();
        return true;
    }

    auto file_path = std::filesystem::current_path() / L"html" / file;
    if(!std::filesystem::exists(file_path)) {
        _RERRW(L"[SchemaHandler] Requested file does not exist: ", file_path.wstring());
        state->error = true;
        callback->Continue();
        return true;
    }

    auto stream = std::ifstream(file_path, std::ios::in | std::ios::binary);
    if(!stream.is_open()) {
        _RERRW(L"[SchemaHandler] Failed to open requested file: ", file_path.wstring());
        state->error = true;
        callback->Continue();
        return true;
    }

    auto size = std::filesystem::file_size(file_path);
    state->data.resize(size, '\0');
    stream.read(state->data.data(), size);
    stream.close();

    if(state->data.empty()) {
        _RERRW(L"[SchemaHandler] Requested file is empty: ", file_path.wstring());
        state->error = true;
        state->mime = "text/plain";
        callback->Continue();
        return true;
    }

    auto ext = file_path.extension().string(); // todo log all of this
    if(ext == ".js") {
        state->mime = "text/javascript";
    } 
    else if(ext == ".html") {
        state->mime = "text/html";
    }
    else if(ext == ".css") {
        state->mime = "text/css";
    }
    else {
        state->mime = "text/plain";
    }

    _RDEBUGW(L"[SchemaHandler] Serving file ", file_path.wstring(), L" with mime type ", state->mime.c_str());
    callback->Continue();
    return true;
}

void SchemaHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) {
    response->SetMimeType(state->mime);
    response->SetStatus(state->error ? 404 : 200);
    response_length = state->error ? 0 : state->data.length();
}

bool SchemaHandler::ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) {
    if(!bytes_to_read || state->error) return false;
    size_t remaining = state->data.size() - state->offset;
    size_t to_copy = std::min<size_t>(remaining, bytes_to_read);
    memcpy(data_out, state->data.data() + state->offset, to_copy);
    state->offset += to_copy;
    bytes_read = static_cast<int>(to_copy);
    _RDEBUG("[SchemaHandler] ReadResponse: requested ", bytes_to_read, " bytes, returning ", bytes_read, " bytes, ", (state->data.size() - state->offset), " bytes remaining");
    return to_copy > 0;
}

void SchemaHandler::Cancel() {}
