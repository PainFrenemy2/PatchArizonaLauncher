#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений для функций форматирования строк
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

// Структура конфигурации
struct PluginSettings {
    char target_ip[64];
    int target_port;
};

// Глобальные настройки (дефолтные значения)
static PluginSettings g_Settings = { "127.0.0.1", 7777 };

// ПОЛНЫЙ ОРИГИНАЛЬНЫЙ СПИСОК ВШИТЫХ IP-АДРЕСОВ (Вы можете менять их прямо здесь)
const char* DEFAULT_IP_POOL[] = {
    "51.91.215.125",   "185.253.34.52",   "176.31.233.153",  "164.132.201.89",
    "149.202.88.119",  "188.165.5.201",   "146.59.209.106",  "188.165.6.206",
    "51.68.244.102",   "51.254.84.40",    "147.135.229.229", "92.222.49.114",
    "185.244.173.35",  "94.23.154.0",     "54.36.150.219",   "188.165.3.143",
    "188.165.138.225", "164.132.119.7",   "185.139.68.64",   "51.38.207.210",
    "5.39.108.48",     "5.39.108.49",     "5.39.108.50",     "5.39.108.51",
    "5.39.108.52",     "5.39.108.53",     "5.39.108.54",     "5.39.108.55",
    "164.132.219.35",  "141.94.184.104",  "141.94.184.105",  "141.94.184.106",
    "141.94.184.107",  "141.94.184.108",  "141.94.184.109",  "141.94.184.110",
    "141.94.184.111",  "54.36.82.238",    "51.38.207.237",   "188.165.59.228",
    "188.165.60.162",  "178.32.98.14",    "178.32.97.38",    "141.95.190.138",
    "141.95.190.139",  "141.95.190.140",  "141.95.190.141",  "141.95.190.142",
    "141.95.190.143",  "141.95.190.144",  "141.95.190.145",  "141.95.190.146",
    "141.95.190.147",  "141.95.190.148",  "54.38.117.72",    "54.38.117.73",
    "54.38.117.74",    "54.38.117.75",    "54.38.117.76",    "54.38.117.77",
    "54.38.117.78",    "54.38.117.79",    "141.95.234.16",   "141.95.234.17",
    "141.95.234.18",   "141.95.234.19",   "141.95.234.20",   "141.95.234.21",
    "141.95.234.22",   "141.95.234.23",   "127.0.0.1"
};
const int IP_POOL_SIZE = sizeof(DEFAULT_IP_POOL) / sizeof(DEFAULT_IP_POOL[0]);

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
bool LoadPluginConfig() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    
    char iniPath[MAX_PATH];
    sprintf_s(iniPath, "%s\\server_connect.ini", currentDir);

    // Проверяем существование файла настроек
    DWORD attribs = GetFileAttributesA(iniPath);
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        return false; // Файла нет, используем вшитый пулл
    }

    g_Settings.target_port = GetPrivateProfileIntA("Settings", "port", 7777, iniPath);
    GetPrivateProfileStringA("Settings", "ip", "127.0.0.1", g_Settings.target_ip, sizeof(g_Settings.target_ip), iniPath);
    return true;
}

// Сетевой поток, выполняющий подключение
void __cdecl ConnectionWorkerThread(void* pParams) {
    Sleep(1500); // Задержка для стабильной прогрузки сетевых интерфейсов игры

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        _endthread();
        return;
    }

    // Если файл .ini существует и успешно прочитан
    if (LoadPluginConfig()) {
        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket != INVALID_SOCKET) {
            sockaddr_in serverAddress;
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(g_Settings.target_port);
            inet_pton(AF_INET, g_Settings.target_ip, &serverAddress.sin_addr);

            connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
            closesocket(clientSocket);
        }
    } 
    // Если файла настроек нет, плагин опрашивает весь вшитый оригинальный пулл IP по очереди
    else {
        for (int i = 0; i < IP_POOL_SIZE; i++) {
            SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket != INVALID_SOCKET) {
                sockaddr_in serverAddress;
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(7777); // Стандартный игровой порт
                inet_pton(AF_INET, DEFAULT_IP_POOL[i], &serverAddress.sin_addr);

                connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
                closesocket(clientSocket);
            }
        }
    }

    WSACleanup();
    _endthread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // Запуск оригинального CRT потока сборки
        _beginthread(ConnectionWorkerThread, 0, NULL);
    }
    return TRUE;
}
