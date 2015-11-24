// debuggee.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

int main()
{
    printf("hello this is parent. pid = %u \n", GetCurrentProcessId());


    STARTUPINFO			si = { 0 };
    PROCESS_INFORMATION pi = { 0 };    
    wchar_t* target = L"c:\\windows\\System32\\notepad.exe";
	if (TRUE != CreateProcessW(
					target, 
					NULL, 
					NULL, 
					NULL, 
					FALSE, 
					NULL, 
					NULL, 
					NULL, 
					&si, 
					&pi))
	{
        return -1;
	}
    printf("child notepad created. pid = %u \n", pi.dwProcessId);
    getchar();

    TerminateProcess(pi.hProcess, 0);
    return 0;
}

