#include "assert.h"
#include "log.h"

#include <libloaderapi.h>
#include <processthreadsapi.h>

// from: WinUser.h
#define MB_OK 0x00000000L
#define MB_ICONHAND 0x00000010L
#define MB_ICONERROR MB_ICONHAND

void nutil_error(const char* err)
{
    Log::error("Fatal error: {}", err);

    typedef int (*tMessageBoxA)(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);

    HMODULE hUser32 = LoadLibraryA("user32.dll");
    if (!hUser32)
        return;

    tMessageBoxA MessageBoxA = reinterpret_cast<tMessageBoxA>(GetProcAddress(hUser32, "MessageBoxA"));
    if (!MessageBoxA)
        return;

    exit(0);
    MessageBoxA(0, err, "fatal error occurred", MB_ICONERROR | MB_OK);
}
