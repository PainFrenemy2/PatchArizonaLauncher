#include "main.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// --- КОНФИГУРАЦИЯ IP И ПОРТА ---
// Вы можете указать абсолютно любой IP-адрес и порт. 
// Теперь длина строки не имеет значения, так как код компилируется из исходника.
const char* target_ip = "185.71.65.56"; 
const int target_port = 7777;

typedef HRESULT(WINAPI* PFN_D3DXAssembleShader)(LPCSTR, UINT, const void*, void*, DWORD, void*, void*);
static PFN_D3DXAssembleShader pOriginalShader = nullptr;

// Реализация прокси-функции для d3dx9_43.dll
extern "C" __declspec(dllexport) HRESULT WINAPI D3DXAssembleShader(
    LPCSTR pSrcData, UINT SrcDataLen, const void* pDefines, 
    void* pInclude, DWORD Flags, void* pShader, void* pErrorMsgs) 
{
    if (!pOriginalShader) {
        char systemPath[MAX_PATH];
        GetSystemDirectoryA(systemPath, MAX_PATH);
        strcat_s(systemPath, "\\d3dx9_43.dll");
        HMODULE hOriginal = LoadLibraryA(systemPath);
        if (hOriginal) {
            pOriginalShader = (PFN_D3DXAssembleShader)GetProcAddress(hOriginal, "D3DXAssembleShader");
        }
    }
    
    if (pOriginalShader) {
        return pOriginalShader(pSrcData, SrcDataLen, pDefines, pInclude, Flags, pShader, pErrorMsgs);
    }
    return S_OK;
}

// Поток сетевого подключения (улучшенный аналог оригинальной логики)
DWORD WINAPI NetworkConnectionThread(LPVOID lpParam) {
    Sleep(1500); // Небольшая задержка для стабильной загрузки игры
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock != INVALID_SOCKET) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(target_port);
        inet_pton(AF_INET, target_ip, &serverAddr.sin_addr);

        // Выполняем подключение к целевому серверу
        connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Тут при необходимости можно дописать логику отправки (send) или приема (recv) пакетов
        
        closesocket(sock);
    }
    WSACleanup();
    return 0;
}

// Главная точка входа плагина
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // Запуск сетевого потока
        CreateThread(NULL, 0, NetworkConnectionThread, NULL, 0, NULL);
    }
    return TRUE;
}
