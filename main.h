#pragma once
#include <windows.h>

// Определение структуры настроек (нужно для main.cpp)
struct PluginSettings {
    char target_ip[16];
    int target_port;
};

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

// Прототипы внутренних функций плагина
bool LoadPluginConfig();
void __cdecl ConnectionWorkerThread(void* pParams);
