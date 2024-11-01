#pragma once

#include <stdint.h>

namespace mem
{
#ifdef _AMD64_
    /*
        get pointer to RIP relative address
    */
    __forceinline unsigned __int64 getPtr(unsigned __int64 addr, unsigned __int32 offset = 0)
    {
        return addr + *(__int32*)(addr + offset) + offset + sizeof(__int32);
    }
    /*
        read RIP relative pointer
    */
    __forceinline unsigned __int64 readPtr(unsigned __int64 addr, unsigned __int32 offset = 0)
    {
        return *reinterpret_cast<unsigned __int64*>(getPtr(addr, offset));
    }

#endif

    /*
        checks if first 4 bytes match last 4 bytes of addr
    */
    __forceinline bool isUninitializedAddr(__int64 addr)
    {
        return static_cast<unsigned __int32>(addr >> 32) == static_cast<unsigned __int32>(addr & 0xFFFFFFFF);
    }

    /*
        Scans for a pattern on the specified module handle
    */
    intptr_t findPattern(void* modHandle, const char* pattern, const char* mask, bool isDirectRef = false);

    /*
        converts data to little endian
    */
    template <typename T> __forceinline void toLittleEndian(unsigned char* dest, T value)
    {
        for (size_t i = 0; i < sizeof(value); ++i)
            dest[i] = (value >> (8 * i)) & 0xFF;
    }

    /*
        remotely allocates memory after an address
    */
    intptr_t allocNearBaseAddr(void* procHandle, intptr_t peAddr, intptr_t baseAddr, unsigned long long lSize);
}
