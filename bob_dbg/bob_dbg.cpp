/**
 * @file    main.cpp
 * @brief   Debugger application's entry point.
 *
 * @author  Yonhgwhan, Noh (fixbrain@gmail.com)
 * @date    2015.11.17 11:17 created.
 * @copyright All rights reserved by Yonghwan, Noh.
**/

#include "stdafx.h"
#include <string>
#include <Windows.h>
#include <Psapi.h>

#include "cpu_helper.h"

#define     _pause  _getch()

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
    
    
static bool _initial_bp = false;

typedef struct DEBUGGEE
{
    bool        _initial_bp_hit;
    uint32_t    _pid;

    // CREATE_PROCESS_DEBUG_INFO 
    HANDLE      _file_handle;
    HANDLE      _proc_handle;
    HANDLE      _thread_handle;
    LPVOID      _base_of_image;
    LPVOID      _thread_local_base;
    /*LPTHREAD_START_ROUTINE*/ 
    void*       _start_address;
    std::wstring _image_name;

    // break point
    ch_param    _bp_param;  // todo : bp 여러개 처리하기
    void*       _bp_addr;
    uint8_t     _opcode;
    
} *PDEBUGGEE;

static DEBUGGEE _debuggees[12] = { {false, 0}, };

/// @brief
int main()
{
    // create process with /dbg
    STARTUPINFO			si = {0};
	PROCESS_INFORMATION pi = {0};
    wchar_t* target = L".\\debuggee.exe";

	if (TRUE != CreateProcessW(
					target, 
					NULL, 
					NULL, 
					NULL, 
					FALSE, 
					CREATE_NEW_CONSOLE | DEBUG_PROCESS,
					NULL, 
					NULL, 
					&si, 
					&pi))
	{
        log("CreateProcess() failed. gle = %u", GetLastError());
		return -1;
	}

    log("process created. pid = %u", pi.dwProcessId);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

    DEBUG_EVENT debug_event = {0};	
	while (true)
	{
		if (TRUE != WaitForDebugEvent(&debug_event, 100)) continue;
        
        DWORD continue_status = DBG_CONTINUE;

		switch (debug_event.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
            {
                PDEBUGGEE dbge = NULL;
                for (int i = 0; i < sizeof(_debuggees) / sizeof(DEBUGGEE); ++i)
                {
                    if (_debuggees[i]._pid == debug_event.dwProcessId)
                    {
                        dbge = &_debuggees[i];
                    }
                }
                if (NULL == dbge) break;
                
                LPEXCEPTION_DEBUG_INFO edi = &debug_event.u.Exception;
                switch (edi->ExceptionRecord.ExceptionCode)
                {
                case EXCEPTION_BREAKPOINT:
                    if (true != dbge->_initial_bp_hit)
                    {
                        dbge->_initial_bp_hit = true;
                        log(
                            "\n"
                            "[EXCEPTION_DEBUG_EVENT] \n"\
                            "   img = %ws \n"\
                            "   pid = %u, tid = %u \n"\
                            "   + initial bp triggered at 0x%llx", 
                            dbge->_image_name.c_str(),
                            debug_event.dwProcessId, 
                            debug_event.dwThreadId,
                            edi->ExceptionRecord.ExceptionAddress);

                        // 
                        // install breakpoint at ep.
                        // 

                        dbge->_bp_param = { 0 };
                        HANDLE thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, debug_event.dwThreadId);
                        if (NULL == thread_handle)
                        {
                            _ASSERTE(!"oops");
                            break;
                        }

                        dbge->_bp_param.hthread = thread_handle;
                        dbge->_bp_param.hproc = dbge->_proc_handle;
                        dbge->_bp_addr = dbge->_start_address;
                        bool bp_ret = set_break_point(
                                            &dbge->_bp_param,
                                            (DWORD_PTR)dbge->_bp_addr,
                                            &dbge->_opcode);
                        if (true != bp_ret)
                        {
                            _ASSERTE(true == bp_ret);
                            break;
                        }

                        continue_status = DBG_EXCEPTION_HANDLED;
                        log("   + break point installed at 0x%llx", dbge->_bp_addr);                        
                        _pause;
                    }
                    else
                    {
                        log(
                            "\n"
                            "[EXCEPTION_DEBUG_EVENT] \n"\
                            "   img = %ws \n"\
                            "   pid = %u, tid = %u \n"\
                            "   + break point at 0x%llx", 
                            dbge->_image_name.c_str(),
                            debug_event.dwProcessId,
                            debug_event.dwThreadId,
                            edi->ExceptionRecord.ExceptionAddress);

                        // 
                        // restore opcode and resume
                        //
                        if (edi->ExceptionRecord.ExceptionAddress != dbge->_bp_addr)
                        {
                            _ASSERTE(!"oops");
                            break;
                        }
                                                
                        dbge->_bp_param.context.ContextFlags  = CONTEXT_ALL;
                        if (TRUE != GetThreadContext(
                                            dbge->_bp_param.hthread, 
                                            &dbge->_bp_param.context))
                        {
                            _ASSERTE(!"oops!");
                            break;
                        }
                        if (true != clear_break_point(
                                            &dbge->_bp_param, 
                                            (DWORD_PTR)dbge->_bp_addr, 
                                            dbge->_opcode, true))
                        {
                            _ASSERTE(!"oops");
                            break;
                        }

                        continue_status = DBG_EXCEPTION_HANDLED;
                        log("   + breakpoint un-installed at 0x%llx", dbge->_bp_addr);                        
                        _pause;
                    }
                    break;

                case EXCEPTION_SINGLE_STEP:
                    log(
                        "\n"
                        "[EXCEPTION_DEBUG_EVENT] \n"\
                        "   pid = %u, tid = %u\n"\
                        "   + handle single step at 0x%llx", 
                        debug_event.dwProcessId,
                        debug_event.dwThreadId, 
                        edi->ExceptionRecord.ExceptionAddress);
                    continue_status = DBG_EXCEPTION_NOT_HANDLED;
                    _pause;
                    break;

                default:
                    if (0 != edi->dwFirstChance)
                    {   
                        log("\n"
                            "[EXCEPTION_DEBUG_EVENT]\n"\
                            "   pid = %u, tid = %u\n"\
                            "   + %s (first chance) at 0x%llx",
                            debug_event.dwProcessId,
                            debug_event.dwThreadId, 
                            exception_code_str(edi->ExceptionRecord.ExceptionCode),
                            edi->ExceptionRecord.ExceptionAddress
                            );
                    }
                    else
                    {
                        log("\n"
                            "[EXCEPTION_DEBUG_EVENT]\n"\
                            "   + %s (second chance) at 0x%llx, can't handle anymore.",
                            exception_code_str(edi->ExceptionRecord.ExceptionCode),
                            edi->ExceptionRecord.ExceptionAddress
                            );
                    }

                    continue_status = DBG_EXCEPTION_NOT_HANDLED;
                    break;
                }
                break;
            }
        case CREATE_THREAD_DEBUG_EVENT:
            {
                //str = "CREATE_THREAD_DEBUG_EVENT";
                break;
            }
        case CREATE_PROCESS_DEBUG_EVENT:
            {
                LPCREATE_PROCESS_DEBUG_INFO cpi = &debug_event.u.CreateProcessInfo;
                std::wstring image;
                if (true != get_filepath_by_handle(cpi->hFile, image))
                {
                    image = L"unknown image path";
                }
                
                log("\n"\
                    "[CREATE_PROCESS_DEBUG_EVENT]\n"\
                    "   img = %ws\n"\
                    "   pid = %u, tid = %u, ep = 0x%llx",
                    image.c_str(), 
                    debug_event.dwProcessId,
                    debug_event.dwThreadId,
                    cpi->lpStartAddress
                    );
                
                for (int i = 0; i < sizeof(_debuggees) / sizeof(DEBUGGEE); ++i)
                {
                    if (_debuggees[i]._pid == 0)
                    {
                        _debuggees[i]._initial_bp_hit = 0;
                        _debuggees[i]._pid = debug_event.dwProcessId;

                        _debuggees[i]._file_handle = debug_event.u.CreateProcessInfo.hFile;
                        _debuggees[i]._proc_handle = debug_event.u.CreateProcessInfo.hProcess;
                        _debuggees[i]._thread_handle = debug_event.u.CreateProcessInfo.hThread;
                        _debuggees[i]._base_of_image = debug_event.u.CreateProcessInfo.lpBaseOfImage;
                        _debuggees[i]._thread_local_base = debug_event.u.CreateProcessInfo.lpThreadLocalBase;
                        _debuggees[i]._start_address = debug_event.u.CreateProcessInfo.lpStartAddress;
                        _debuggees[i]._image_name = image;

                        break;
                    }
                }
                break;
            }
        case EXIT_THREAD_DEBUG_EVENT:
            {
                //str = "EXIT_THREAD_DEBUG_EVENT";
                break;
            }
        //case EXIT_PROCESS_DEBUG_EVENT:
        //    {
        //        str = "EXIT_PROCESS_DEBUG_EVENT";
        //        break;
        //    }
        case LOAD_DLL_DEBUG_EVENT:
            {
                break;

                LPLOAD_DLL_DEBUG_INFO lddi = &debug_event.u.LoadDll;
                std::wstring dll_path;
                if (true != get_filepath_by_handle(lddi->hFile, dll_path))
                {
                    dll_path = L"Unknown dll path";
                }

                log("\n"
                    "[LOAD_DLL_DEBUG_EVENT]\n"\
                    "   pid = %u, tid = %u, img = %ws",
                    debug_event.dwProcessId,
                    debug_event.dwThreadId,
                    dll_path.c_str()
                    );
                break;
            }
        case UNLOAD_DLL_DEBUG_EVENT:
            {
                LPUNLOAD_DLL_DEBUG_INFO uddi = &debug_event.u.UnloadDll;
                // todo 3
                break;
            }
        case OUTPUT_DEBUG_STRING_EVENT:
            {
                
                break;
            }
        case RIP_EVENT:
            {
                //str = "RIP_EVENT";
                break;
            }
        default:
            //str = "unknown";
            break;
        }

        


		//> continue_status 값이
		//>
		//>	DBG_CONTINUE
		//>		EXCEPTION_DEBUG_EVENT 인 경우
		//>			모든 exception processing 을 멈추고 스레드를 계속 실행
		//>		EXCEPTION_DEBUG_EVENT 이 아닌 경우 
		//>			스레드 계속 실행 
		//>
		//>	DBG_EXCEPTION_NOT_HANDLED
		//>		EXCEPTION_DEBUG_EVENT 인 경우
		//>			exception processing 을 계속 진행.
		//>			first-chance exception 인 경우 seh 핸들러의 search, dispatch 로직이 실행
		//>			first-chance exception 이 아니라면 프로세스는 종료 됨
		//>		EXCEPTION_DEBUG_EVENT 이 아닌 경우 
		//>			스레드 계속 실행 
        ContinueDebugEvent(debug_event.dwProcessId,debug_event.dwThreadId,continue_status);

		//> debuggee is terminating
		if (EXIT_PROCESS_DEBUG_EVENT == debug_event.dwDebugEventCode)
		{
            int i = 0;
            for (; i < sizeof(_debuggees) / sizeof(DEBUGGEE); ++i)
            {
                if (_debuggees[i]._pid == debug_event.dwProcessId)
                {
                    _debuggees[i]._initial_bp_hit = 0;
                    _debuggees[i]._pid = 0;
                    break;
                }
            }

            if (i == 0)
            {
                log("\n"\
                    "[EXIT_PROCESS_DEBUG_EVENT] \n"\
                    "   img = %ws\n"\
                    "   pid = %u, tid = %u\n"\
                    "   + debuggee is terminated.",
                    _debuggees[i]._image_name.c_str(),
                    debug_event.dwProcessId, debug_event.dwThreadId);
                _pause;
                break;
            }
            // do nothing...
		}
	}

    //log("press any key to terminate...");
    //getchar();

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