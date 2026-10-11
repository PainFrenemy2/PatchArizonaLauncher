#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений компилятора
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

// --- МЕНЯЙТЕ IP И ПОРТ ЗДЕСЬ (Зашивается во все настройки) ---
const char* MY_TARGET_IP = "91.215.87.204"; // Впишите сюда ваш IP (вместо Brainburg)
const int MY_TARGET_PORT = 7777;             // Впишите сюда ваш Порт

// Структура конфигурации
struct PluginSettings {
    char target_ip[64];
    int target_port;
};

// Инициализируем настройки вашим IP по умолчанию
static PluginSettings g_Settings = { "", 7777 };

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

// Загрузка конфигурации из INI файла настроек
void LoadPluginConfig() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    
    char iniPath[MAX_PATH];
    sprintf_s(iniPath, "%s\\server_connect.ini", currentDir);

    // Считываем порт, если файла нет — берем ваш MY_TARGET_PORT
    g_Settings.target_port = GetPrivateProfileIntA("Settings", "port", MY_TARGET_PORT, iniPath);
    
    // Считываем IP, если файла нет — автоматически подставится ваш MY_TARGET_IP
    GetPrivateProfileStringA("Settings", "ip", MY_TARGET_IP, g_Settings.target_ip, sizeof(g_Settings.target_ip), iniPath);
}

// Сетевой поток, выполняющий принудительное подключение к вашему IP
void __cdecl ConnectionWorkerThread(void* pParams) {
    Sleep(2000); // Задержка для стабильной прогрузки сетевых интерфейсов игры

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        _endthread();
        return;
    }

    // Загружаем конфиг (подставит ваш IP из строки 12, если INI файла нет)
    strcpy_s(g_Settings.target_ip, MY_TARGET_IP);
    LoadPluginConfig();

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket != INVALID_SOCKET) {
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(g_Settings.target_port);
        inet_pton(AF_INET, g_Settings.target_ip, &serverAddress.sin_addr);

        // Прямое подключение СТРОГО к вашему IP
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
