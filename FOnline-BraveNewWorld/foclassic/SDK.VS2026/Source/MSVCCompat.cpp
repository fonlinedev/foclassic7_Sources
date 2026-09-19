#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <io.h>

extern "C" FILE* __cdecl _iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" FILE* __cdecl __iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" FILE* __cdecl ___iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" int __cdecl compat_vfprintf(FILE* stream, const char* format, va_list args)
{
    return vfprintf(stream, format, args);
}

extern "C" int __cdecl compat_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    return vsnprintf(buffer, size, format, args);
}

int __cdecl fltk_wopen_compat(const wchar_t* filename, int oflag, int pmode)
{
    return _open_osfhandle(
        (intptr_t)CreateFileW(
            filename,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        ),
        oflag
    );
}

#pragma comment(linker, "/alternatename:?_wopen@@YAHPB_WHH@Z=?fltk_wopen_compat@@YAHPB_WHH@Z")
#pragma comment(linker, "/alternatename:__vsnprintf=_vsnprintf")