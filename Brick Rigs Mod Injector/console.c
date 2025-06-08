#include "console.h"
#include <windows.h>
#include <stdio.h>

FILE* pStdIn = NULL;
FILE* pStdOut = NULL;
FILE* pStdErr = NULL;

void InitConsole()
{
    AllocConsole();
    freopen_s(&pStdIn, "CONIN$", "r", stdin);
    freopen_s(&pStdOut, "CONOUT$", "w", stdout);
    freopen_s(&pStdErr, "CONOUT$", "w", stderr);
    SetConsoleTitleW(L"Brick Rigs Mod Injector");
    SetConsoleOutputCP(CP_UTF8);
}

void DestroyConsole()
{
    fclose(pStdIn);
    fclose(pStdOut);
    fclose(pStdErr);
    SetStdHandle(STD_INPUT_HANDLE, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, NULL);
    SetStdHandle(STD_ERROR_HANDLE, NULL);
    FreeConsole();
    PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
}
