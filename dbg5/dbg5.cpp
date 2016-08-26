/**
 * @file    main.cpp
 * @brief   Debugger application's entry point.
 *
 * @author  Yonhgwhan, Noh (fixbrain@gmail.com)
 * @date    2015.11.17 11:17 created.
 * @copyright All rights reserved by Yonghwan, Noh.
**/

#include "stdafx.h"
#include <Windows.h>
#include <Psapi.h>
#include <list>
#include <string>
#include <sal.h>

#define     _pause  getchar()

const char* exception_code_str(_In_ DWORD exception_code);
bool get_mapped_file_name(
    _In_ HANDLE process_handle,
    _In_ const void* mapped_addr,
    _Out_ std::wstring& file_name
);

bool get_filepath_by_handle(
    _In_ HANDLE file_handle,
    _Out_ std::wstring& file_path
);


typedef class Thread
{
public:
    Thread() {}
    ~Thread() {}

    DWORD   thread_id;
    HANDLE  hThread;
    LPVOID  lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;

} *PThread;

/// @brief  debuggee process 정보
typedef class Debuggee
{
public: 
    Debuggee() 
    {
    }
    ~Debuggee() {}
   
    DWORD   process_id;
    HANDLE  process_handle;
    HANDLE  thread_handle;
    std::wstring    image;
    
    std::list<Thread>   threads;
}*PDebuggee;





/// @brief
int main()
{
    //Debuggee dbg[12] = { 0 };
    std::list<Debuggee> dbg_list;

    
    // create process with /dbg
    STARTUPINFO			si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    //wchar_t* target = L"c:\\windows\\system32\\calc.exe";
    wchar_t* target = L".\\debuggee.exe";

    if (TRUE != CreateProcessW(
                    target,
                    NULL,
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_NEW_CONSOLE | CREATE_SUSPENDED | DEBUG_PROCESS,
                    NULL,
                    NULL,
                    &si,
                    &pi))
    {
        printf("CreateProcess() failed. gle = %u \n", GetLastError());
        return -1;
    }
    printf("Press any key to continue...\n");
    _pause;
    ResumeThread(pi.hThread);

    // start debug loop
    DEBUG_EVENT debug_event = { 0 };
    while (true)
    {
        if (TRUE != WaitForDebugEvent(&debug_event, 100)) continue;

        DWORD continue_status = DBG_CONTINUE;

        switch (debug_event.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            {
                LPCREATE_PROCESS_DEBUG_INFO info = (LPCREATE_PROCESS_DEBUG_INFO)&debug_event.u.CreateProcessInfo;
                
                std::wstring image;
                if (true != get_filepath_by_handle(info->hFile, image))
                {
                    image = L"unknown image path";
                }


                // debuggee 할당/추가
                Debuggee dbg;
                dbg.process_id = debug_event.dwProcessId;
                dbg.process_handle = info->hProcess;
                dbg.thread_handle = info->hThread;
                dbg.image = image;
                dbg_list.push_back(dbg);
                break;
            }

        case CREATE_THREAD_DEBUG_EVENT:
            {
                LPCREATE_THREAD_DEBUG_INFO info = (LPCREATE_THREAD_DEBUG_INFO)&debug_event.u.CreateThread;
                Thread thread;
                thread.thread_id = GetThreadId(info->hThread);
                thread.hThread = info->hThread;
                thread.lpStartAddress = info->lpStartAddress;
                thread.lpThreadLocalBase = info->lpThreadLocalBase;

                for (auto dbg : dbg_list)
                {
                    if (dbg.process_id == debug_event.dwProcessId)
                    {
                        dbg.threads.push_back(thread);
                    }
                }
                break;
            }
        case EXIT_THREAD_DEBUG_EVENT:
            {
                LPEXIT_THREAD_DEBUG_INFO info = (LPEXIT_THREAD_DEBUG_INFO)&debug_event.u.ExitThread;
                for (auto dbg : dbg_list)
                {
                    if (dbg.process_id == debug_event.dwProcessId)
                    {
                        std::list<Thread>::iterator tit = dbg.threads.begin();
                        for (; tit != dbg.threads.end(); ++tit)
                        {
                            if (tit->thread_id == debug_event.dwThreadId)
                            {
                                dbg.threads.erase(tit);
                                break;
                            }
                        }
                    }
                }
                break;
            }
        default:
            break;
        }

        if (EXIT_PROCESS_DEBUG_EVENT == debug_event.dwDebugEventCode)
        {  
            std::list<Debuggee>::iterator it = dbg_list.begin();
            for (; it != dbg_list.end(); ++it)
            {
                if (it->process_id == debug_event.dwProcessId)
                {
                    /// 정보 출력
                    printf("proc handle=0x%p \n", it->process_handle);
                    printf("main thrad handle=0x%p \n", it->thread_handle);
                    break;  // debuggee 가 종료되면, debug loop 도 끝내야 한다.
                }
            }
            dbg_list.erase(it);
        }

        ContinueDebugEvent(
            debug_event.dwProcessId,
            debug_event.dwThreadId,
            continue_status);

        if (dbg_list.empty())
        {
            break;
        }

    }

    printf("\nPress any key to terminate...\n");

    // todo 2, 정보 몽땅 출력하기
    _pause;
    
    return 0;
}


const char* exception_code_str(_In_ DWORD exception_code)
{
    switch (exception_code)
    {
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_GUARD_PAGE: return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_INVALID_HANDLE: return "EXCEPTION_INVALID_HANDLE";
    //case EXCEPTION_POSSIBLE_DEADLOCK: return "EXCEPTION_POSSIBLE_DEADLOCK";
    default:
        return "UNKNOWN EXCEPTION CODE";
    }

}


/**
 * @brief	file_handle 로 file path 를 구하는 함수
 * @param	
 * @see		http://msdn.microsoft.com/en-us/library/windows/desktop/aa366789(v=vs.85).aspx
 * @remarks	NtQueryObject() 를 사용하는게 더 좋을 것 같기도 함, 권한 문제가 발생 할 수도 있을것 같으나
 * @remarks 확인해본적 없음
 * @code		
 * @endcode	
 * @return	
**/
bool
get_filepath_by_handle(
	_In_ HANDLE file_handle, 
	_Out_ std::wstring& file_path
	)
{
	_ASSERTE(NULL != file_handle);
	_ASSERTE(INVALID_HANDLE_VALUE != file_handle);	
	if (NULL == file_handle || INVALID_HANDLE_VALUE == file_handle) return false;
	
	LARGE_INTEGER file_size = {0};
	if (TRUE != GetFileSizeEx(file_handle, &file_size))
	{
		return false;
	}

	if (0 == file_size.QuadPart)
	{
		return false;
	}
	
    bool succeeded = false;
    HANDLE hcfm = NULL;
    LPVOID mvo = NULL;

    do
    {
        hcfm = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 1, NULL);
	    if (NULL == hcfm)
	    {
            break;
	    }

        mvo = MapViewOfFile(hcfm, FILE_MAP_READ, 0, 0, 1);
	    if (NULL == mvo)
	    {   
            break;
	    }

	    std::wstring nt_device_name;
	    if (true != get_mapped_file_name(
					    GetCurrentProcess(), 
					    mvo, 
					    nt_device_name))
	    {
            break;
	    }

	    /*if (true != nt_name_to_dos_name(nt_device_name.c_str(), file_path))
	    {
		    log_err "nt_name_to_dos_name( nt name = %s )", nt_device_name.c_str() log_end
		    return false;
	    }*/
        file_path = nt_device_name.c_str();
        succeeded = true;
    } while (false);
    
    if (NULL != hcfm) CloseHandle(hcfm);
    if (NULL != mvo) UnmapViewOfFile(mvo);
    return succeeded;
}

/**
 * @brief	wrapper function for GetMappedFileName() 
 * @param	
 * @see		http://msdn.microsoft.com/en-us/library/windows/desktop/ms683195(v=vs.85).aspx
 * @remarks	
 * @code		
 * @endcode	
 * @return	true if succeeded.
 * @return	file_name is nt device name (e.g. Device\HarddiskVolume2\Windows\System32\drivers\etc\hosts)
 * @return	if you want use dos device name, use nt_name_to_dos_name() function.
**/
bool 
get_mapped_file_name(
	_In_ HANDLE process_handle, 
	_In_ const void* mapped_addr, 
	_Out_ std::wstring& file_name
	)
{
	bool		ret = false;
	DWORD		ret_cch_buf = 0;
	DWORD		cch_buf = MAX_PATH;
	wchar_t*	buf = NULL; 
	
	for(;;)
	{
		if (NULL != buf) free(buf);

		buf = (wchar_t*) malloc((cch_buf + 1) * sizeof(wchar_t));	// add NULL 
		if (NULL == buf) 
		{
			//log_err 
			//	"insufficient memory, malloc( %u )", 
			//	(cch_buf + 1) * sizeof(wchar_t) 
			//log_end
			return false;
		}

		ret_cch_buf = GetMappedFileNameW(
							process_handle, 
							const_cast<void*>(mapped_addr), 
							buf, 
							cch_buf
							);
		if (0 == ret_cch_buf)
		{
			//log_err 
			//	"GetMappedFileNameW( process handle = 0x%08x, addr = 0x%p ), gle = %u",
			//	process_handle, 
			//	mapped_addr, 
			//	GetLastError()
			//log_end
			
			break;
		}

		if (ret_cch_buf < cch_buf)
		{
			// OK!
			ret = true;
			buf[ret_cch_buf] = L'\0';
			break;
		}
		else if (ret_cch_buf == cch_buf)
		{
			// we need more buffer
			cch_buf *= 2;
			continue;
		}
		else
		{
			//log_err 
			//	"unexpected ret_cch_buf(%u) : cch_buf(%u), GetMappedFileNameW()",
			//	ret_cch_buf,
			//	cch_buf
			//log_end
			break;
		}
	}

	if(true == ret) file_name = buf;
	
	free(buf); buf = NULL;
	return ret;
}