#pragma once
#include <iostream>
#include <sstream>

#ifndef _RLOGGER
#define _RLOGGER

template<typename... Args>
inline void __rlog(const char* prefix, Args&&... args) {
    std::ostringstream oss;
    oss << prefix;
    (oss << ... << args);
    oss << "\033[0m" << std::endl;
    std::cerr << oss.str();
}

template<typename... Args>
inline void __rlogw(const wchar_t* prefix, Args&&... args) {
    std::wostringstream oss;
    oss << prefix;
    (oss << ... << args);
    oss << L"\033[0m" << std::endl;
    std::wcerr << oss.str();
}

#ifdef _DEBUG

#define _RLOG(...) __rlog("\033[32m[INFO] ", __VA_ARGS__)
#define _RERR(...) __rlog("\033[31m[ERROR] ", __VA_ARGS__)
#define _RWARN(...) __rlog("\033[33m[WARNING] ", __VA_ARGS__)
#define _RDEBUG(...) __rlog("\033[35m[DEBUG] ", __VA_ARGS__)

#define _RLOGW(...) __rlogw(L"\033[32m[INFO] ", __VA_ARGS__)
#define _RERRW(...) __rlogw(L"\033[31m[ERROR] ", __VA_ARGS__)
#define _RWARNW(...) __rlogw(L"\033[33m[WARNING] ", __VA_ARGS__)
#define _RDEBUGW(...) __rlogw(L"\033[35m[DEBUG] ", __VA_ARGS__)

#else 

#define _RLOG(...)
#define _RERR(...)
#define _RWARN(...)
#define _RDEBUG(...)

#define _RLOGW(...)
#define _RERRW(...)
#define _RWARNW(...)
#define _RDEBUGW(...)

#endif

#endif // !_RLOGGER
