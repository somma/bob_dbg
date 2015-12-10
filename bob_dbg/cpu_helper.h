/**----------------------------------------------------------------------------
 * cpu_helper.h
 *-----------------------------------------------------------------------------
 * 
 *-----------------------------------------------------------------------------
 * All rights reserved by Noh,Yonghwan (fixbrain@gmail.com, unsorted@msn.com)
 *-----------------------------------------------------------------------------
 * 2014:2:3 13:53 created
**---------------------------------------------------------------------------*/
#pragma once

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
