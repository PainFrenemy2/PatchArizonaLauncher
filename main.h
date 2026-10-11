#pragma once
#include <windows.h>

// Экспортируем функцию один в один, как в оригинальном бинарнике
extern "C" __declspec(dllexport) HRESULT WINAPI D3DXAssembleShader(
    LPCSTR pSrcData,
    UINT SrcDataLen,
    const void* pDefines,
    void* pInclude,
    DWORD Flags,
    void* pShader,
    void* pErrorMsgs
);
