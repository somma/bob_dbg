#pragma once

#include <Windows.h>

typedef struct _ch_param
{
	HANDLE	hproc;
	HANDLE	hthread;
	CONTEXT	context;
} ch_param;


bool set_break_point(_In_ ch_param* param, _In_ DWORD_PTR address, _Out_ unsigned char* opcode);
bool clear_break_point(_In_ ch_param* param, _In_ DWORD_PTR address, _In_ unsigned char opcode, _In_ bool reset_eip);
bool set_single_step(_In_ ch_param* param);
bool clear_single_step(_In_ ch_param* param);

bool set_last_branch_enable(_In_ ch_param* param);