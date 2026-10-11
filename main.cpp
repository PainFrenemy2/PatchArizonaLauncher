#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений компилятора
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread

#pragma comment(lib, "ws2_32.lib")

// Структура конфигурации
struct PluginSettings {
    char target_ip[64];
    int target_port;
};

// ЕДИНСТВЕННЫЙ ЦЕЛЕВОЙ СЕРВЕР (Для смены IP просто отредактируйте этот адрес ниже)
static PluginSettings g_Settings = { "91.215.87.204", 7777 };

// Указатель на оригинальную функцию экспорта DirectX 9
typedef HRESULT(WINAPI* PFN_D3DXAssembleShader)(LPCSTR, UINT, const void*, void*, DWORD, void*, void*);
static PFN_D3DXAssembleShader pOriginalShader = nullptr;

// Экспортируем функцию один в один для бесперебойного запуска игры
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

// Сетевой поток, выполняющий принудительное подключение к единственному IP
void __cdecl ConnectionWorkerThread(void* pParams) {
    Sleep(1500); // Задержка для стабильной прогрузки сетевых интерфейсов игры

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        _endthread();
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket != INVALID_SOCKET) {
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(g_Settings.target_port);
        inet_pton(AF_INET, g_Settings.target_ip, &serverAddress.sin_addr);

        // Прямое подключение к зашитому IP
        connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
        closesocket(clientSocket);
    }

    WSACleanup();
    _endthread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // Запуск потока коннекта
        _beginthread(ConnectionWorkerThread, 0, NULL);
    }
    return TRUE;
}
