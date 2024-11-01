#pragma once

#include <stdint.h>

namespace PE {
	/*
		gets the pointer to a section header from a virtual address
	*/
	intptr_t getSectionFromVA(intptr_t peAddr, intptr_t vaAddr);

	/*
		gets actual address from a virtual address
	*/
	intptr_t getAddrFromVA(intptr_t peAddr, intptr_t vaAddr);

	/*
		adds dlls to import table of a process
		(only works on newly created suspended processes)
	*/
	bool attachDLLs(void* procHandle, intptr_t peAddr, int dllCount, const char** dlls);
}