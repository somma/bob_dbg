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


#define     _pause  getchar()

/// @brief  debuggee process 정보
typedef struct DEBUGGEE
{
    bool    busy;
    DWORD   process_id;
    HANDLE  process_handle;
    HANDLE  thread_handle;
    // todo 1, child thread  리스트 추가하기
}*PDEBUGGEE;






/// @brief
int main()
{
    DEBUGGEE dbg[12] = { 0 };
    
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
                
                PDEBUGGEE pdbg = NULL;
                for (int i = 0; i < sizeof(dbg); i++)
                {
                    if (dbg[i].busy != true)
                    {
                        pdbg = &dbg[i];
                        break;
                    }
                }
                if (NULL == pdbg)
                {
                    return -1;      // 아몰랑.
                }

                pdbg->process_id = debug_event.dwProcessId;
                pdbg->process_handle = info->hProcess;
                pdbg->thread_handle = info->hThread;
                pdbg->busy = true;
                break;
            }
        default:
            break;
        }

        if (EXIT_PROCESS_DEBUG_EVENT == debug_event.dwDebugEventCode)
        {  
            PDEBUGGEE pdbg = NULL;
            for (int i = 0; i < sizeof(dbg); i++)
            {
                if (dbg[i].process_id == debug_event.dwProcessId)
                {
                    pdbg = &dbg[i];
                    break;
                }
            }
            if (NULL == pdbg)
            {
                return -1;      // 아몰랑.
            }
            pdbg->busy = false;


            if (debug_event.dwProcessId == dbg[0].process_id)
            {
                /// 정보 출력
                printf("proc handle=0x%p \n", dbg[0].process_handle);
                printf("main thrad handle=0x%p \n", dbg[0].thread_handle);
                break;  // debuggee 가 종료되면, debug loop 도 끝내야 한다.
            }
        }

        ContinueDebugEvent(
            debug_event.dwProcessId,
            debug_event.dwThreadId,
            continue_status);

    }

    printf("\nPress any key to terminate...\n");

    // todo 2, 정보 몽땅 출력하기
    _pause;
    
    return 0;
}