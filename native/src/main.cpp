#include <iostream>
#include <filesystem>
#include "PMApp.h"

#ifdef WIN32

void EnableConsoleColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;

    // Enable virtual terminal processing for ANSI codes
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CefMainArgs main_args(GetModuleHandle(NULL));
    #ifdef _DEBUG
    bool allocated_console = false;
    if(!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
        allocated_console = true;
    }
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    EnableConsoleColors();
    #endif
#else
int main(int argc, char** argv) {
    CefMainArgs main_args(argc, argv);
#endif
    CefRefPtr<PMApp> app(new PMApp());
    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }
    auto pwd = std::filesystem::current_path();
    CefSettings settings;
    settings.no_sandbox = true;
    settings.log_severity = LOGSEVERITY_ERROR;
    
    CefString(&settings.resources_dir_path).FromWString(pwd.wstring()); // may not have worked bc of project settings
    CefString(&settings.locales_dir_path).FromWString((pwd / "locales").wstring()); // also check hanging pointers in virtual getters
    CefString(&settings.locale).FromWString(L"en-US");
    CefInitialize(main_args, settings, app, nullptr);
    CefRunMessageLoop();
    CefShutdown();

#if defined(WIN32) && defined(_DEBUG)
    if(allocated_console) {
        FreeConsole();
    }
#endif

    return 0;
}