#include "memory.h"

#include <minwindef.h>

#include <Psapi.h>
#include <processthreadsapi.h>

/*
    compares the passed ptr against a signature
*/
bool patternCompare(const char* data, const char* patt, const char* mask)
{
    for (; *mask; ++mask, ++data, ++patt)
    {
        if (*mask == 'x' && *data != *patt)
            return false;
    }
    return true;
}

/*
    scan a specific module for a pattern
*/
intptr_t mem::findPattern(void* modHandle, const char* pattern, const char* mask, bool isDirectRef)
{
    /*
        TO-DO:
            use sections instead of scanning the whole image
    */
    MODULEINFO modInfo = {0};
    GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(modHandle), &modInfo, sizeof(MODULEINFO));

    DWORD64 baseAddr = reinterpret_cast<DWORD64>(modHandle);
    DWORD64 modSize = modInfo.SizeOfImage;

    for (unsigned __int64 i = 0; i < modSize; i++)
    {
        if (patternCompare(reinterpret_cast<char*>(baseAddr + i), pattern, mask))
        {
            if (isDirectRef)
            {
                auto curr = baseAddr + i;
                auto start = reinterpret_cast<intptr_t*>(baseAddr + i);

#if _WIN64
                return mem::getPtr(baseAddr + i, 1);
#else
                if (*reinterpret_cast<unsigned char*>(start) == 0xE8)
                    return curr + *(intptr_t*)(curr + 1) + 1 + sizeof(intptr_t);
                else
                    return *reinterpret_cast<intptr_t*>(baseAddr + i + 1);
#endif
            }
            return baseAddr + i;
        }
    }
    return 0;
}
