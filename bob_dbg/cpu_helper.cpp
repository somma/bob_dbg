#include "stdafx.h"
#include "cpu_helper.h"

#define TF_BIT				0x100
#define BREAK_OPCODE		0xCC
#define DR7_TRACE_BRANCH	0x200
#define DR7_LAST_BRANCH		0x100

bool
set_break_point(
	_In_ ch_param* param,
	_In_ DWORD_PTR address,
	_Out_ unsigned char* opcode
	)
{
	_ASSERTE(NULL != param);
	_ASSERTE(0 != address);
	_ASSERTE(NULL != opcode);
	if (NULL == param || 0 == address || NULL == opcode) return false;

	SIZE_T cb_rw = 0;
	unsigned char temp = 0;
	BOOL ret = ReadProcessMemory(
		param->hproc,
		(LPCVOID)address,
		&temp,
		sizeof(unsigned char),
		&cb_rw
		);

	if (TRUE != ret || cb_rw != sizeof(unsigned char))
	{
		log(
			"ReadProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u",
			param->hproc,
			address,
			GetLastError()
			);
		return false;
	}

	*opcode = temp;

	if (temp == BREAK_OPCODE)
	{
		#pragma TODO("같은 주소에 서로 다른 타입의 브레이크 포인트를 설치하려고 하는 경우 해결하기")
		log(
			"hprocess = 0x%08x, address =0x%p, breakpoint is already installed",
			param->hproc,
			address
			);
		return true;
	}

	MEMORY_BASIC_INFORMATION mbi = { 0 };
	if (0 == VirtualQueryEx(
		param->hproc,
		(LPCVOID)address,
		&mbi,
		sizeof(mbi)))
	{
		log(
			"VirtualQueryEx( hprocess = 0x%08x, address = 0x%p ), gle = %u",
			param->hproc,
			address,
			GetLastError());
		return false;
	}

	if (TRUE != VirtualProtectEx(
		param->hproc,
		mbi.BaseAddress,
		mbi.RegionSize,
		PAGE_EXECUTE_READWRITE,
		&mbi.Protect))
	{
		log(
			"VirtualProtectEx( hprocess = 0x%08x, address = 0x%p, size = %u, PAGE_EXECUTE_READWRITE ), gle = %u",
			param->hproc,
			mbi.BaseAddress,
			mbi.RegionSize,
			GetLastError()
			);
		return false;
	}

	do
	{
		temp = BREAK_OPCODE;
		ret = WriteProcessMemory(
			param->hproc,
			(LPVOID)address,
			&temp,
			sizeof(unsigned char),
			&cb_rw);

		if (TRUE != ret || cb_rw != sizeof(unsigned char))
		{
			log(
				"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u",
				param->hproc,
				address,
				GetLastError()
				);

			ret = FALSE;
			break;
		}

		if (TRUE != FlushInstructionCache(param->hproc, (LPCVOID)address, sizeof(unsigned char)))
		{
			log(
				"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u",
				param->hproc,
				address,
				GetLastError()
				);
		}
	} while (false);

	VirtualProtectEx(param->hproc, mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);
	return (TRUE == ret) ? true : false;
}