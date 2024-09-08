#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include "consts.hpp"

DWORD GetProcessID(const std::wstring& processName) {
    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (!_wcsicmp(pe32.szExeFile, processName.c_str())) {
                CloseHandle(hProcessSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return 0;
}

DWORD_PTR GetModuleBaseAddress(DWORD processID, const std::wstring& moduleName) {
    MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);

    if (hModuleSnap == INVALID_HANDLE_VALUE) return 0;

    if (Module32First(hModuleSnap, &me32)) {
        do {
            if (!_wcsicmp(me32.szModule, moduleName.c_str())) {
                CloseHandle(hModuleSnap);
                return reinterpret_cast<DWORD_PTR>(me32.modBaseAddr);
            }
        } while (Module32Next(hModuleSnap, &me32));
    }

    CloseHandle(hModuleSnap);
    return 0;
}

void WriteMemory(HANDLE hProcess, DWORD_PTR address, int value) {
    WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), &value, sizeof(value), nullptr);
}


int main() {
    DWORD processID = GetProcessID(L"cs2.exe");
    if (processID == 0) {
        std::cerr << "cs2.exe process is not running!" << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        std::cerr << "Error accessing process cs2.exe" << std::endl;
        return 1;
    }

    DWORD_PTR clientBaseAddress = GetModuleBaseAddress(processID, L"client.dll");
    if (clientBaseAddress == 0) {
        std::cerr << "client.dll module not found" << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    DWORD_PTR forceJumpAddress = clientBaseAddress + OFFSET_JUMP;
    bool jump = false;

    while (true) {
        if (GetAsyncKeyState(SPACE_KEY) & 0x8000) {
            Sleep(SLEEP_INTERVAL);
            WriteMemory(hProcess, forceJumpAddress, jump ? FORCE_JUMP_OFF : FORCE_JUMP_ON);
            jump = !jump;
            std::cout << "Force jump toggled." << std::endl;
        }
        Sleep(SLEEP_INTERVAL);
    }

    CloseHandle(hProcess);
    return 0;
}

