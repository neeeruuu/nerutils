#include "memory.h"

#include <minwindef.h>

#include <Psapi.h>
#include <memoryapi.h>
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

// 64kb granularity
// https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
#define ALLOC_GRANULARITY 0x10000

#define FOUR_GB 0xFFFFFFFF

intptr_t mem::allocNearBaseAddr(void* procHandle, intptr_t peAddr, intptr_t baseAddr, unsigned long long lSize)
{
    MEMORY_BASIC_INFORMATION mbi;

    intptr_t lastAddr = baseAddr;
    for (;; lastAddr = reinterpret_cast<intptr_t>(mbi.BaseAddress) + mbi.RegionSize)
    {
        memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION));

        if (VirtualQueryEx(procHandle, reinterpret_cast<void*>(lastAddr), &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
            break;

        if ((mbi.RegionSize & 0xfff) == 0xfff)
            break;

        if (mbi.State != MEM_FREE)
            continue;

        intptr_t addr = (reinterpret_cast<intptr_t>(mbi.BaseAddress) > baseAddr)
                            ? reinterpret_cast<DWORD64>(mbi.BaseAddress)
                            : baseAddr;
        addr = addr + (ALLOC_GRANULARITY - 1) & ~(ALLOC_GRANULARITY - 1);

        if ((addr + lSize - 1 - peAddr) > FOUR_GB)
            return 0;

        for (; addr < reinterpret_cast<DWORD64>(mbi.BaseAddress) + mbi.RegionSize; addr += ALLOC_GRANULARITY)
        {
            void* allocated = VirtualAllocEx(procHandle, reinterpret_cast<void*>(addr), lSize, MEM_RESERVE | MEM_COMMIT,
                                             PAGE_READWRITE);
            if (!allocated)
                continue;

            if ((addr + lSize - 1 - baseAddr) > FOUR_GB)
                return 0;

            return addr;
        }
    }

    return 0;
}
