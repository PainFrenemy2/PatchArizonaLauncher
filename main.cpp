#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений компилятора MSVC
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

// --- НАСТРОЙКА ВАШЕГО IP И ПОРТА НАМЕРТВО ---
const char* TARGET_SERVER_IP = "192.168.0.139"; 
const int TARGET_SERVER_PORT = 7777;

// Структура для получения информации о модуле в памяти
struct MY_MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD  SizeOfImage;
    LPVOID EntryPoint;
};

// Динамический импорт функции из kernel32 для скрытности структуры
typedef BOOL(WINAPI* PFN_K32GetModuleInformation)(HANDLE, HMODULE, MY_MODULEINFO*, DWORD);

// --- БЛОК 1: ОБХОД ПЕРЕВЕРОК VORBISFILE.DLL ---
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

    // Сканируем сигнатуру (39 ?? E8 01 00 00) внутри vorbisfile.dll
    for (DWORD i = 0; i < size - 6; i++) {
        if (baseAddr[i] == 0x39 && baseAddr[i + 2] == 0xE8 && baseAddr[i + 3] == 0x01 && baseAddr[i + 4] == 0x00 && baseAddr[i + 5] == 0x00) {
            DWORD oldProtect;
            // Открываем доступ на запись строго в место патча (+6 байт)
            if (VirtualProtect(&baseAddr[i + 6], 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                baseAddr[i + 6] = 0xEB; // Делаем условный переход безусловным (JMP)
                VirtualProtect(&baseAddr[i + 6], 1, oldProtect, &oldProtect);
                break;
            }
        }
    }
}

// --- БЛОК 2: ПРИНУДИТЕЛЬНАЯ ПОДМЕНА СЕРВЕРА В SA-MP ---
void __cdecl MainExecutionThread(void* pParams) {
    Sleep(2000); // Ожидание полной прогрузки игры и её модулей

    // Шаг 1: Применяем патч памяти vorbisfile
    PatchVorbisFile();

    // Шаг 2: Ищем SA-MP в памяти процесса
    HMODULE hSamp = GetModuleHandleA("samp.dll");
    if (!hSamp) {
        _endthread();
        return;
    }

    // Ожидаем инициализацию сетевого объекта SA-MP 0.3.7-R3 (Смещение: 0x26E8DC)
    BYTE** pSampNetworkObj = (BYTE**)((BYTE*)hSamp + 0x26E8DC);
    while (!*pSampNetworkObj) {
        Sleep(100);
    }

    BYTE* networkStruct = *pSampNetworkObj;

    // Открываем доступ к памяти сетевой структуры SA-MP для записи
    DWORD oldProtect;
    if (VirtualProtect(networkStruct + 0x30, 256, PAGE_READWRITE, &oldProtect)) {
        // Перезаписываем IP-адрес (смещение +0x30) и Порт (смещение +0x235) напрямую в памяти
        strcpy((char*)(networkStruct + 0x30), TARGET_SERVER_IP);
        *(int*)(networkStruct + 0x235) = TARGET_SERVER_PORT;
        
        VirtualProtect(networkStruct + 0x30, 256, oldProtect, &oldProtect);
    }

    _endthread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // Запуск основного потока хукинга памяти
        _beginthread(MainExecutionThread, 0, NULL);
    }
    return TRUE;
}
