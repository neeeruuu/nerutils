#include "pe.h"

#include <minwindef.h>
#include <winnt.h>

#include <memoryapi.h>

#include <corecrt_malloc.h>

#include <nerutils/memory.h>

intptr_t PE::getSectionFromVA(intptr_t peAddr, intptr_t vaAddr)
{
    /*
        get PE headers
    */
    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(peAddr);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(peAddr + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    /*
        iterate headers until finding the one that contains the vaAddr
    */
    intptr_t secHeadersStart = peAddr + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER header =
            reinterpret_cast<PIMAGE_SECTION_HEADER>(secHeadersStart + (i * sizeof(IMAGE_SECTION_HEADER)));
        intptr_t sectionVA = header->VirtualAddress;

        if ((vaAddr >= sectionVA) && (vaAddr <= sectionVA + header->SizeOfRawData))
            return reinterpret_cast<intptr_t>(header);
    }
    return 0;
}

intptr_t PE::getAddrFromVA(intptr_t peAddr, intptr_t vaAddr)
{
    PIMAGE_SECTION_HEADER header = reinterpret_cast<PIMAGE_SECTION_HEADER>(PE::getSectionFromVA(peAddr, vaAddr));
    if (!header)
        return 0;
    return peAddr + (header->PointerToRawData + (vaAddr - header->VirtualAddress));
}

/*
    same method as https://www.x86matthew.com/view_post?id=import_dll_injection
    but on x64 and using IMAGE_THUNK_DATA

    TO-DO:
        cleanup on fail
*/
bool PE::attachDLLs(void* procHandle, intptr_t peAddr, int dllCount, const char** dlls)
{
    /*
        get PE headers
    */
    IMAGE_DOS_HEADER dosHeader;
    if (!ReadProcessMemory(procHandle, reinterpret_cast<void*>(peAddr), &dosHeader, sizeof(IMAGE_DOS_HEADER), NULL))
        return false;
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(procHandle, reinterpret_cast<void*>(peAddr + dosHeader.e_lfanew), &ntHeaders,
                           sizeof(IMAGE_NT_HEADERS), NULL))
        return false;
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        return false;

    /*
        calculate number of original imports and size of new import directory
        allocate import directory
    */

    int originalImportCount =
        ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    DWORD idtSize = (originalImportCount + dllCount) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    IMAGE_IMPORT_DESCRIPTOR* idt = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(malloc(idtSize));
    if (!idt)
        return false;

    /*
        since we're gonna be allocating memory that needs to be after the base address
        we need to skip the actual process's code and data
    */
    intptr_t allocBase = peAddr;
    allocBase += ntHeaders.OptionalHeader.BaseOfCode;
    allocBase += ntHeaders.OptionalHeader.SizeOfCode;
    allocBase += ntHeaders.OptionalHeader.SizeOfInitializedData;
    allocBase += ntHeaders.OptionalHeader.SizeOfUninitializedData;

    /*
        if the program already had imports copy those into our IDT after the space reserved for the new DLLs
    */
    if (ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != 0)
    {
        size_t readBytes = 0;
        bool hasRead = ReadProcessMemory(procHandle, reinterpret_cast<void*>(peAddr + ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress), &idt[dllCount], originalImportCount * sizeof(IMAGE_IMPORT_DESCRIPTOR), &readBytes);
		if (!hasRead || readBytes < originalImportCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))
			return false;
    }

    /*
        iterate all dlls to be loaded
    */
    for (int i = 0; i < dllCount; i++)
    {
        /*
            allocate path
        */
        const char* dllPath = dlls[i];
        void* remoteDllPath =
            reinterpret_cast<void*>(mem::allocNearBaseAddr(procHandle, peAddr, allocBase, strlen(dllPath) + 1));
        if (!remoteDllPath)
            return false;
        if (!WriteProcessMemory(procHandle, remoteDllPath, dllPath, strlen(dllPath) + 1, NULL))
            return false;

        /*
            define thunk data, allocate and copy remotely
        */
        IMAGE_THUNK_DATA itd[2];
#ifdef _WIN64
        itd[0].u1.Ordinal = IMAGE_ORDINAL_FLAG64 + 1;
#else
        itd[0].u1.Ordinal = IMAGE_ORDINAL_FLAG32 + 1;
#endif
        itd[1].u1.Ordinal = 0;

        void* remoteITD = reinterpret_cast<void*>(mem::allocNearBaseAddr(procHandle, peAddr, allocBase, sizeof(itd)));
        if (!remoteITD)
            return false;
        if (!WriteProcessMemory(procHandle, remoteITD, itd, sizeof(itd), 0))
            return false;

        /*
            allocate and write IAT (same as ITD)
        */
        void* remoteIAT = reinterpret_cast<void*>(mem::allocNearBaseAddr(procHandle, peAddr, allocBase, sizeof(itd)));
        if (!remoteIAT)
            return false;
        if (!WriteProcessMemory(procHandle, remoteIAT, itd, sizeof(itd), 0))
            return false;

        /*
            define DLL import descriptor and copy into IDT
        */
        IMAGE_IMPORT_DESCRIPTOR iid;
        iid.OriginalFirstThunk = DWORD(reinterpret_cast<intptr_t>(remoteITD) - peAddr);
        iid.TimeDateStamp = 0;
        iid.ForwarderChain = 0;
        iid.Name = DWORD(reinterpret_cast<intptr_t>(remoteDllPath) - peAddr);
        iid.FirstThunk = DWORD(reinterpret_cast<intptr_t>(remoteIAT) - peAddr);

        memcpy(&idt[i], &iid, sizeof(iid));
    }

    /*
        allocate and copy import description table remotely
    */
    void* remoteIDT = reinterpret_cast<void*>(mem::allocNearBaseAddr(procHandle, peAddr, allocBase, idtSize));
    if (!remoteIDT)
        return false;
    if (!WriteProcessMemory(procHandle, remoteIDT, idt, idtSize, NULL))
        return false;

    /*
        update NT headers so that they use the new import descriptor table
    */
    ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress =
        DWORD(reinterpret_cast<DWORD64>(remoteIDT) - peAddr);
    ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = idtSize;

    /*
        write modified NT headers back to process
    */
    DWORD ntProtection;
    DWORD ntProtection2;
    if (VirtualProtectEx(procHandle, reinterpret_cast<void*>(peAddr + dosHeader.e_lfanew), sizeof(IMAGE_NT_HEADERS),
                         PAGE_EXECUTE_READWRITE, &ntProtection) == 0)
        return false;
    if (WriteProcessMemory(procHandle, reinterpret_cast<void*>(peAddr + dosHeader.e_lfanew), &ntHeaders,
                           sizeof(IMAGE_NT_HEADERS), NULL) == 0)
        return false;
    if (VirtualProtectEx(procHandle, reinterpret_cast<void*>(peAddr + dosHeader.e_lfanew), sizeof(IMAGE_NT_HEADERS),
                         ntProtection, &ntProtection2) == 0)
        return false;

    return true;
}
