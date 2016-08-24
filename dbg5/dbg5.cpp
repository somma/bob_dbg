// dbg5.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main()
{
    STARTUPINFO			si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    wchar_t* target = L"c:\\windows\\system32\\calc.exe";
    if (TRUE != CreateProcessW(target,NULL,NULL,NULL,FALSE,
                               CREATE_NEW_CONSOLE | DEBUG_PROCESS,
                               NULL,NULL,&si,&pi))
    {return -1;}
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DEBUG_EVENT debug_event = { 0 };
    while (true)
    {
        if (TRUE != WaitForDebugEvent(&debug_event, 100)) continue;
        DWORD continue_status = DBG_CONTINUE;
        ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, continue_status);
    }
    return 0;
}
