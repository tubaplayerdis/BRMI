#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "console.h"
#include <shlwapi.h>
#include <psapi.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Shlwapi.lib")

#pragma region HELPER_FUNCTIONS
HRESULT DownloadFileWithWinINet(const WCHAR* szUrl, const WCHAR* szFileName)
{
    HINTERNET hInternetSession = NULL;
    HINTERNET hUrlHandle = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwBytesRead = 0;
    BYTE buffer[4096]; //Hopefully this wont go over
    HRESULT hr = E_FAIL;

    //init win"I"Net
    hInternetSession = InternetOpen(L"BRMIApp", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if (hInternetSession == NULL)
    {
        fprintf(stderr, "InternetOpen failed: %lu\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    printf("WinINet session opened.\n");

    // Open URL. FLAG MEANINGS
    // INTERNET_FLAG_RELOAD: Force download from the server, not cache
    // INTERNET_FLAG_KEEP_CONNECTION: Maintain connection if possible
    // INTERNET_FLAG_SECURE: Use SSL/TLS for HTTPS
    hUrlHandle = InternetOpenUrl(hInternetSession, szUrl, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_SECURE, 0);

    if (hUrlHandle == NULL)
    {
        fprintf(stderr, "InternetOpenUrl failed: %lu\n", GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    printf("URL opened successfully: %ls\n", szUrl);

    //Create the file
    hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFile failed for %ls: %lu\n", szFileName, GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    printf("Local file created: %ls\n", szFileName);

    // 4. Read data from the URL and write to the file
    printf("Downloading...\n");
    while (InternetReadFile(hUrlHandle, buffer, sizeof(buffer), &dwBytesRead) && dwBytesRead > 0)
    {
        DWORD dwBytesWritten;
        if (!WriteFile(hFile, buffer, dwBytesRead, &dwBytesWritten, NULL))
        {
            fprintf(stderr, "WriteFile failed: %lu\n", GetLastError());
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
        if (dwBytesWritten != dwBytesRead)
        {
            fprintf(stderr, "Warning: Mismatched bytes written. Expected %lu, wrote %lu\n", dwBytesRead, dwBytesWritten);
        }
        // Maybe print progress.
        printf("Read %lu bytes. Total written: %lu\n", dwBytesRead, (ULONG)GetFileSize(hFile, NULL));
    }

    // Check for errors after the loop (dwBytesRead will be 0 on success or error)
    if (GetLastError() != ERROR_SUCCESS)
    {
        fprintf(stderr, "InternetReadFile failed or completed with error: %lu\n", GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    printf("Download complete.\n");
    hr = S_OK; // Success!

cleanup:
    // 5. Close handles in reverse order of opening
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        printf("Local file handle closed.\n");
    }
    if (hUrlHandle != NULL)
    {
        InternetCloseHandle(hUrlHandle);
        printf("URL handle closed.\n");
    }
    if (hInternetSession != NULL)
    {
        InternetCloseHandle(hInternetSession);
        printf("WinINet session closed.\n");
    }

    return hr;
}

unsigned long SFindProcessID(const wchar_t* processName)
{
    PROCESSENTRY32 entry = { 0 };
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (wcscmp(entry.szExeFile, processName) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return 0;
}
#pragma endregion

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    //TODO: Add command line arguments. one for the link the other for the dll name


    //Initalize basic variables
    DWORD dwBRCIHandle = 0;
    PLARGE_INTEGER dwBRCISize = 0;
    char failedInternet = 0;
    char failedDownload = 0;

    //Initalize the console
    InitConsole();

    //
	printf("Downloading Brick Rigs Command Interpreter.\n");
    if (!InternetCheckConnection(L"https://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0)) {
        printf("WARNING: Internet connection failed! DEFAULTING on pre-existing file.");
        failedInternet = 1;
    }

    if (!failedInternet)
    {
        HRESULT res = DownloadFileWithWinINet(L"https://aaronwilk.dev/pages/brci/BrickRigsCommandInterpreter.dll", L"BrickRigsCommandInterpreter.dll");
        if (res != S_OK) {
            printf("The download experienced an error!\n");
            LPVOID msgBuffer = NULL;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuffer, 0, NULL);
            printf("Error from download: %s\n", res, (char*)msgBuffer);
            LocalFree(msgBuffer);

            printf("WARNING: Download failed! DEFAULTING on pre-existing file.");
            failedDownload = 1;
        }
    }

    Sleep(100);

  
    HRESULT hret = CreateFile(
        L"BrickRigsCommandInterpreter.dll",
        GENERIC_READ,          // We only need read access to get its size
        FILE_SHARE_READ,       // Allow other processes to read the file
        NULL,                  // Default security attributes
        OPEN_EXISTING,         // File must exist
        FILE_ATTRIBUTE_NORMAL, // Normal file attributes
        NULL
    );

    GetFileSizeEx(hret, &dwBRCISize);

    wchar_t errorMsg[256] = L"BrickRigsCommandInterpreter.dll was NOT found. Please inject again. See the following codes:";

    if (failedInternet) {
        wcscat(errorMsg, L"\nInternet Connection FAILED");
    }
    if (failedDownload) {
        wcscat(errorMsg, L"\nFile DOWNLOAD FAILED");
    }

    if (dwBRCISize == 0) {
        MessageBox(GetActiveWindow(), errorMsg, L"Missing file", MB_OK);
        DestroyConsole();
        return 1;
    }

    DWORD pid = SFindProcessID(L"BrickRigs-Win64-Shipping.exe");
    printf("Process ID: %lu", pid);
    if (pid == 0) {
        MessageBox(GetActiveWindow(), L"Brick Rigs was not found! Please inject again with Brick Rigs running.", L"Brick Rigs not found", MB_OK);
        DestroyConsole();
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        DWORD errorCode = GetLastError(); // Get the last Win32 error
        wchar_t errorMessage[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, sizeof(errorMessage) / sizeof(wchar_t), NULL);
        printf("Win32 Error: %d - %ls\n", errorCode, errorMessage);
        MessageBox(GetActiveWindow(), L"FAILED to open Brick Rigs! Please try agian", L"Injection ERROR", MB_OK);
        DestroyConsole();
        return 1;
    }

    const char* dllPath[MAX_PATH];
    GetFullPathNameA("BrickRigsCommandInterpreter.dll", MAX_PATH, dllPath, NULL);

    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;
    //Check if BrickRigsCommandInterpreter.dll is Already Loaded
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                if (StrStr(szModName, L"BrickRigsCommandInterpreter.dll") != NULL) {
                    MessageBox(GetActiveWindow(), L"BrickRigsCommandInterpreter.dll is already injected! It can be uninjected by pressing /", L"Already Injected!", MB_OK);
                    CloseHandle(hProcess);
                    DestroyConsole();
                    return 1;
                }
            }
        }
    }

    LPVOID alloc = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!alloc) {
        MessageBox(GetActiveWindow(), L"FAILED to allocate memory in Brick Rigs. Please try agian", L"Injection ERROR", MB_OK);
        CloseHandle(hProcess);
        DestroyConsole();
        return 1;
    }

    if (!WriteProcessMemory(hProcess, alloc, dllPath, strlen(dllPath) + 1, NULL)) {
        MessageBox(GetActiveWindow(), L"FAILED to write DLL path to Brick Rigs. Please try agian", L"Injection ERROR", MB_OK);
        VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DestroyConsole();
        return 1;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPVOID loadLib = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");

    if (hKernel32 == NULL || loadLib == NULL) {
        MessageBox(GetActiveWindow(), L"Failed to find kernel dll or LoadLibraryA", L"Injection ERROR", MB_OK);
        VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DestroyConsole();
        return 1;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)loadLib, alloc, 0, 0);
    if (!hThread) {
        MessageBox(GetActiveWindow(), L"FAILED to create remote thread. Please try agian", L"Injection ERROR", MB_OK);
        VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DestroyConsole();
        return 1;
    }

    if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
    {
        DWORD errorCode = GetLastError(); // Get the last Win32 error
        wchar_t errorMessage[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, sizeof(errorMessage) / sizeof(wchar_t), NULL);
        printf("Win32 Error: %d - %ls\n", errorCode, errorMessage);
        MessageBox(GetActiveWindow(), L"FAILED to inject. Please try agian", L"Injection ERROR", MB_OK);
        VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DestroyConsole();
        return 1;
    }

    VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    MessageBox(GetActiveWindow(), L"Successfully Injected!", L"Injection SUCCESS", MB_OK); //Check both of these for spelling
    printf("Successfully Injected!\n");
    
    DestroyConsole();
	return 0;
}