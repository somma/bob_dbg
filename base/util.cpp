#include "stdafx.h"
#include "util.h"

/// @brief 
void log(_In_z_ const char* fmt, _In_ ... )
{
    if (NULL == fmt) return;

    char log_buffer[2048];
    size_t remain = sizeof(log_buffer);
    char* pos = log_buffer;
    va_list args;

    va_start(args, fmt);
    HRESULT hRes = StringCbVPrintfExA(
                        pos,
                        remain,
                        &pos,
                        &remain,
                        0,
                        fmt,
                        args
                        );
    if (S_OK != hRes)
    {
        // invalid character 가 끼어있는 경우 발생 할 수 있음
        StringCbPrintfExA(
            pos,
            remain,
            &pos,
            &remain,
            0,
            "invalid function call parameters"
            );
    }
    va_end(args);

    // line feed
    StringCbPrintfExA(pos, remain, &pos, &remain, 0, "\n");
    printf("%s", log_buffer);
    OutputDebugStringA(log_buffer);
}
