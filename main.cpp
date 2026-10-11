#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений компилятора MSVC
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread
#include <cstdio>
#include <cstring>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

// --- ЕДИНЫЙ БЕЛЫЙ СПИСОК РАЗРЕШЕННЫХ СЕРВЕРОВ (IP:PORT) ---
// Самая ПЕРВАЯ строка в этом списке ВСЕГДА является основным дефолтным сервером.
// Если игра или /rec попытается подключиться к левому серверу, плагин сбросит коннект на этот первый адрес.
const char* ALLOWED_SERVERS[] = {
    "91.215.87.204:7777",   // Основной дефолтный сервер
    "185.169.134.45:7777",    // Второй разрешенный сервер
    "127.0.0.1:7778"        // Третий разрешенный сервер (можно добавлять сколько угодно через запятую)
};
const int ALLOWED_SERVERS_SIZE = sizeof(ALLOWED_SERVERS) / sizeof(ALLOWED_SERVERS[0]);

// Структура конфигурации (буфер в 64 байта исправляет ошибку типов компилятора)
struct PluginSettings {
    char target_ip[64];
    int target_port;
};

static PluginSettings g_Settings = { "127.0.0.1", 7777 };

struct MY_MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD  SizeOfImage;
    LPVOID EntryPoint;
};

typedef BOOL(WINAPI* PFN_K32GetModuleInformation)(HANDLE, HMODULE, MY_MODULEINFO*, DWORD);

// Вывод красивого лога в отладчик Windows
void ShowAuthorCredits() {
    OutputDebugStringA("\n============================================================\n");
    OutputDebugStringA("[ConnectPatch] Plugin successfully initialized!\n");
    OutputDebugStringA("[ConnectPatch] Developed by: PainFrenemy / Code: by null-code\n");
    OutputDebugStringA("==============================================================\n\n");
}

// --- БЛОК 1: ОБХОД ПРОВЕРОК VORBISFILE.DLL ---
void PatchVorbisFile() {
    HMODULE hVorbis = GetModuleHandleA("vorbisfile.dll");
    if (!hVorbis) return;

    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    if (!hKernel) return;

    PFN_K32GetModuleInformation pGetModuleInfo = (PFN_K32GetModuleInformation)GetProcAddress(hKernel, "K32GetModuleInformation");
    if (!pGetModuleInfo) return;

    MY_MODULEINFO modInfo;
    if (!pGetModuleInfo(GetCurrentProcess(), hVorbis, &modInfo, sizeof(modInfo))) return;

    BYTE* baseAddr = (BYTE*)modInfo.lpBaseOfDll;
    DWORD size = modInfo.SizeOfImage;

    for (DWORD i = 0; i < size - 6; i++) {
        if (baseAddr[i] == 0x39 && baseAddr[i + 2] == 0xE8 && baseAddr[i + 3] == 0x01 && baseAddr[i + 4] == 0x00 && baseAddr[i + 5] == 0x00) {
            DWORD oldProtect;
            if (VirtualProtect(&baseAddr[i + 6], 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                baseAddr[i + 6] = 0xEB; 
                VirtualProtect(&baseAddr[i + 6], 1, oldProtect, &oldProtect);
                break;
            }
        }
    }
}

// Проверка: находится ли текущий скомбинированный сервер игры в белом списке?
bool IsServerAllowed(const char* current_ip, int current_port) {
    if (!current_ip || current_ip[0] == '\0') {
        return false; 
    }

    char current_combined[128];
    sprintf(current_combined, "%s:%d", current_ip, current_port);

    for (int i = 0; i < ALLOWED_SERVERS_SIZE; i++) {
        if (strcmp(current_combined, ALLOWED_SERVERS[i]) == 0) {
            return true; 
        }
    }
    return false; 
}

// Функция безопасного разделения строки "IP:PORT" для записи в память игры
void ParseAndApplyDefaultServer(char* game_ip_dest, int* game_port_dest) {
    char temp_pool[128];
    strcpy(temp_pool, ALLOWED_SERVERS[0]); // Фикс: явно берем первый дефолтный сервер [0]

    char* token = strtok(temp_pool, ":");
    if (token != nullptr) {
        strcpy(game_ip_dest, token); 
        token = strtok(nullptr, ":");
        if (token != nullptr) {
            *game_port_dest = atoi(token); // Фикс типов: переводим строку в число
        }
    }
}

// --- БЛОК 2: ПЕРЕХВАТ, ПРОВЕРКА И ПАРСИНГ АДРЕСОВ ---
void __cdecl MainExecutionThread(void* pParams) {
    HMODULE hSamp = nullptr;
    while (!hSamp) {
        hSamp = GetModuleHandleA("samp.dll");
        Sleep(200);
    }

    PatchVorbisFile();
    ShowAuthorCredits();

    BYTE** pSampNetworkObj = (BYTE**)((BYTE*)hSamp + 0x26E8DC);
    while (!pSampNetworkObj || !(*pSampNetworkObj)) {
        Sleep(200);
    }

    // Бесконечный цикл жесткого контроля сетевого адреса
    while (true) {
        BYTE* networkStruct = *pSampNetworkObj;
        if (networkStruct) {
            DWORD oldProtect;
            if (VirtualProtect(networkStruct + 0x30, 256, PAGE_READWRITE, &oldProtect)) {
                
                char* current_game_ip = (char*)(networkStruct + 0x30);
                int* current_game_port = (int*)(networkStruct + 0x235);

                // Проверяем текущую связку IP:PORT игры по нашему белому списку
                if (!IsServerAllowed(current_game_ip, *current_game_port)) {
                    // Если связка неверная — принудительно разделяем и вшиваем дефолтный первый сервер
                    ParseAndApplyDefaultServer(current_game_ip, current_game_port);
                }
                
                VirtualProtect(networkStruct + 0x30, 256, oldProtect, &oldProtect);
            }
        }
        Sleep(100); 
    }

    _endthread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        _beginthread(MainExecutionThread, 0, NULL);
    }
    return TRUE;
}
