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

bool
clear_break_point(
	_In_ ch_param* param,
	_In_ DWORD_PTR address,
	_In_ unsigned char opcode,
	_In_ bool reset_eip
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

	if (temp != BREAK_OPCODE)
	{
		log(
			"hprocess = 0x%08x, address = 0x%p, opcode = 0x%02x (not BREAK_OPCODE!)",
			param->hproc,
			address,
			temp
			);
		return false;
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
			GetLastError()
			);
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
			"VirtualProtectEx( hprocess = 0x%08x, address = 0x%p, size = %u, PAGE_EXECUTE_READWRITE ) gle = %u",
			param->hproc,
			mbi.BaseAddress,
			mbi.RegionSize,
			GetLastError()
			);
		return false;
	}

	do
	{
		ret = WriteProcessMemory(
			param->hproc,
			(LPVOID)address,
			&opcode,
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

	if (true == reset_eip)
	{
		_ASSERTE(NULL != param->hthread);
		if (NULL != param->hthread)
		{
#if defined(_AMD64_)
			param->context.Rip--;
#elif defined(_X86_)
			param->context.Eip--;
#else
#error !!unsupported architecture!!
#endif			
			if (TRUE != SetThreadContext(param->hthread, &param->context))
			{
				log(
					"SetThreadContext(hthread=0x%08x), gle = %u",
					param->hthread,
					GetLastError()
					);
			}
		}
	}

	return (TRUE == ret) ? true : false;
}