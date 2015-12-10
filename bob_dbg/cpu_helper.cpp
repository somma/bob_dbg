/**----------------------------------------------------------------------------
 * CpuHelper.cpp
 *-----------------------------------------------------------------------------
 * 
 *-----------------------------------------------------------------------------
 * All rights reserved by Noh,Yonghwan (fixbrain@gmail.com, unsorted@msn.com)
 *-----------------------------------------------------------------------------
 * 2014:2:3 15:01 created
**---------------------------------------------------------------------------*/
#include "stdafx.h"
#include "cpu_helper.h"

#define TF_BIT				0x100
#define BREAK_OPCODE		0xCC
#define DR7_TRACE_BRANCH	0x200
#define DR7_LAST_BRANCH		0x100


/**
 * @brief	
 * @param	
 * @see		
 * @remarks	
 * @code		
 * @endcode	
 * @return	
**/
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
		//log_err
		//	L"ReadProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
		//	param->hproc, 
		//	address, 
		//	GetLastError()
		//log_end
		return false;
	}

	*opcode = temp;

	if (temp == BREAK_OPCODE)
	{
		//#pragma TODO("같은 주소에 서로 다른 타입의 브레이크 포인트를 설치하려고 하는 경우 해결하기")
		//log_info
		//	L"hprocess = 0x%08x, address = 0x%p, breakpoint is already installed", 
		//	param->hproc, 
		//	address
		//log_end
		return true;
	}

	MEMORY_BASIC_INFORMATION mbi = {0};
	if (0 == VirtualQueryEx(
					param->hproc, 
					(LPCVOID)address, 
					&mbi, 
					sizeof(mbi)))
	{
		//log_err
		//	L"VirtualQueryEx( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
		//	param->hproc, 
		//	address, 
		//	GetLastError()
		//log_end
		return false;
	}

	if (TRUE != VirtualProtectEx(
					param->hproc, 
					mbi.BaseAddress,
					mbi.RegionSize,
					PAGE_EXECUTE_READWRITE, 
					&mbi.Protect))
	{
		//log_err
		//	L"VirtualProtectEx( hprocess = 0x%08x, address = 0x%p, size = %u, PAGE_EXECUTE_READWRITE ), gle = %u", 
		//	param->hproc, 
		//	mbi.BaseAddress, 
		//	mbi.RegionSize,
		//	GetLastError()
		//log_end
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
			//log_err
			//	L"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
			//	param->hproc, 
			//	address, 
			//	GetLastError()
			//log_end		

			ret = FALSE;
			break;
		}
		
		if (TRUE != FlushInstructionCache(param->hproc, (LPCVOID)address, sizeof(unsigned char)))
		{
			//log_err
			//	L"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
			//	param->hproc, 
			//	address, 
			//	GetLastError()
			//log_end
		}
	} while (false);

	VirtualProtectEx(param->hproc, mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);
	return (TRUE == ret) ? true : false;
}

/**
 * @brief	
 * @param	
 * @see		
 * @remarks	
 * @code		
 * @endcode	
 * @return	
**/
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
		//log_err
		//	L"ReadProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
		//	param->hproc, 
		//	address, 
		//	GetLastError()
		//log_end
		return false;
	}

	if (temp != BREAK_OPCODE)
	{
		//log_info
		//	L"hprocess = 0x%08x, address = 0x%p, opcode = 0x%02x (not BREAK_OPCODE!)", 
		//	param->hproc, 
		//	address, 
		//	temp
		//log_end
		return false;
	}

	MEMORY_BASIC_INFORMATION mbi = {0};
	if (0 == VirtualQueryEx(
					param->hproc, 
					(LPCVOID)address, 
					&mbi, 
					sizeof(mbi)))
	{
		//log_err
		//	L"VirtualQueryEx( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
		//	param->hproc, 
		//	address, 
		//	GetLastError()
		//log_end
		return false;
	}

	if (TRUE != VirtualProtectEx(
					param->hproc, 
					mbi.BaseAddress,
					mbi.RegionSize,
					PAGE_EXECUTE_READWRITE, 
					&mbi.Protect))
	{
		//log_err
		//	L"VirtualProtectEx( hprocess = 0x%08x, address = 0x%p, size = %u, PAGE_EXECUTE_READWRITE ), gle = %u", 
		//	param->hproc, 
		//	mbi.BaseAddress, 
		//	mbi.RegionSize,
		//	GetLastError()
		//log_end
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
			//log_err
			//	L"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
			//	param->hproc, 
			//	address, 
			//	GetLastError()
			//log_end		

			ret = FALSE;
			break;
		}
		
		if (TRUE != FlushInstructionCache(param->hproc, (LPCVOID)address, sizeof(unsigned char)))
		{
			//log_err
			//	L"WriteProcessMemory( hprocess = 0x%08x, address = 0x%p ), gle = %u", 
			//	param->hproc, 
			//	address, 
			//	GetLastError()
			//log_end
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
				//log_err
				//	L"SetThreadContext(hthread=0x%08x), gle = %u",
				//	param->hthread, 
				//	GetLastError()
				//log_end
			}
		}		
	}

	return (TRUE == ret) ? true : false;
}

/**
 * @brief	
 * @param	
 * @see		
 * @remarks	
 * @code		
 * @endcode	
 * @return	
**/
bool 
set_single_step(
	_In_ ch_param* param
	)
{
	_ASSERTE(NULL != param);
	if (NULL == param) return false;

	param->context.EFlags |= TF_BIT;
	if (TRUE != SetThreadContext(param->hthread, &param->context))
	{
		//log_err
		//	L"SetThreadContext(hthread=0x%08x), gle = %u",
		//	param->hthread, 
		//	GetLastError()
		//log_end
		return false;
	}

	return true;
}

/**
 * @brief	
 * @param	
 * @see		
 * @remarks	
 * @code		
 * @endcode	
 * @return	
**/
bool 
clear_single_step(
	_In_ ch_param* param
	)
{
	_ASSERTE(NULL != param);
	if (NULL == param) return false;

	param->context.EFlags &= ~TF_BIT;
	if (TRUE != SetThreadContext(param->hthread, &param->context))
	{
		//log_err
		//	L"SetThreadContext(hthread=0x%08x), gle = %u",
		//	param->hthread, 
		//	GetLastError()
		//log_end
		return false;
	}

	return true;
}


/**
 * @brief	
 * @param	
 * @see		
 * @remarks	
 * @code		
 * @endcode	
 * @return	
**/
bool set_last_branch_enable(_In_ ch_param* param)
{
	_ASSERTE(NULL != param);
	if (NULL == param) return false;

	param->context.EFlags	|= TF_BIT;
//	param->context.Dr7		|= (DR7_LAST_BRANCH | DR7_TRACE_BRANCH);
	param->context.Dr7		|= (DR7_TRACE_BRANCH);
	if (TRUE != SetThreadContext(param->hthread, &param->context))
	{
		//log_err
		//	L"SetThreadContext(hthread=0x%08x), gle = %u",
		//	param->hthread, 
		//	GetLastError()
		//log_end
		return false;
	}

	return true;
}