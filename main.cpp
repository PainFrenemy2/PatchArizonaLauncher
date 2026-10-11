#define WIN32_LEAN_AND_MEAN     // Защита от конфликта Winsock и Windows.h
#define _CRT_SECURE_NO_WARNINGS // Отключение предупреждений компилятора MSVC
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>            // Использование оригинального потокового вызова _beginthread
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

// --- НАСТРОЙКА ВАШЕГО IP И ПОРТА НАМЕРТВО ---
const char* TARGET_SERVER_IP = "91.215.87.204"; 
const int TARGET_SERVER_PORT = 7777;

struct MY_MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD  SizeOfImage;
    LPVOID EntryPoint;
};

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

// --- БЛОК 2: ПЕРЕХВАТ СКРИПТОВ СМЕНЫ IP И /REC ---
void __cdecl MainExecutionThread(void* pParams) {
    // Цикл ожидания полной загрузки samp.dll
    HMODULE hSamp = nullptr;
    while (!hSamp) {
        hSamp = GetModuleHandleA("samp.dll");
        Sleep(100);
    }

    // Патчим vorbisfile
    PatchVorbisFile();

    // Ожидаем инициализацию сетевого объекта SA-MP 0.3.7-R3 (Смещение: 0x26E8DC)
    BYTE** pSampNetworkObj = (BYTE**)((BYTE*)hSamp + 0x26E8DC);
    while (!*pSampNetworkObj) {
        Sleep(100);
    }

    // Бесконечный цикл-хук: жестко удерживает ваш IP в памяти.
    // Если скрипт /rec попытается сбросить IP на оригинальный, плагин мгновенно перезапишет его обратно.
    while (true) {
        BYTE* networkStruct = *pSampNetworkObj;
        if (networkStruct) {
            DWORD oldProtect;
            // Открываем доступ на запись к сетевым структурам адреса (+0x30) и порта (+0x235)
            if (VirtualProtect(networkStruct + 0x30, 256, PAGE_READWRITE, &oldProtect)) {
                
                // Проверяем, если адрес изменился (например, после команды /rec), принудительно возвращаем наш
                if (strcmp((char*)(networkStruct + 0x30), TARGET_SERVER_IP) != 0 || *(int*)(networkStruct + 0x235) != TARGET_SERVER_PORT) {
                    strcpy((char*)(networkStruct + 0x30), TARGET_SERVER_IP);
                    *(int*)(networkStruct + 0x235) = TARGET_SERVER_PORT;
                }
                
                VirtualProtect(networkStruct + 0x30, 256, oldProtect, &oldProtect);
            }
        }
        Sleep(50); // Проверка памяти каждые 50 миллисекунд (не грузит процессор)
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
