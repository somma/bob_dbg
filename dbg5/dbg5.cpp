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
#include <list>

#define     _pause  getchar()

/// @brief  debuggee process 정보
typedef struct DEBUGGEE
{
    DWORD   process_id;
    HANDLE  process_handle;
    HANDLE  thread_handle;
    // todo 1, child thread  리스트 추가하기
}*PDEBUGGEE;






/// @brief
int main()
{
    //DEBUGGEE dbg[12] = { 0 };
    std::list<DEBUGGEE> dbg_list;

    
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
                
                // debuggee 할당/추가
                DEBUGGEE dbg = { 0 };
                dbg.process_id = debug_event.dwProcessId;
                dbg.process_handle = info->hProcess;
                dbg.thread_handle = info->hThread;
                dbg_list.push_back(dbg);
                break;
            }
        default:
            break;
        }

        if (EXIT_PROCESS_DEBUG_EVENT == debug_event.dwDebugEventCode)
        {  
            std::list<DEBUGGEE>::iterator it = dbg_list.begin();
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