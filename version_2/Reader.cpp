#include <windows.h>
#include <stdio.h>
#include "SharedData.h"

#define ID_LABEL 1
#define TIMER_ID 999

HANDLE hMapFile;
SharedData* pBuf;
HWND hLabel;
int last_seq = -1;
int last_counter = -1;

void Cleanup() {
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char buffer[512];

    switch (msg) {
    case WM_CREATE:
        hLabel = CreateWindow("STATIC", "Waiting...", WS_CHILD | WS_VISIBLE, 10, 10, 360, 100, hwnd, (HMENU)ID_LABEL, NULL, NULL);

        hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEM_NAME);
        if (hMapFile) {
            pBuf = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
        }
        SetTimer(hwnd, TIMER_ID, 100, NULL); // Fast polling (100ms) for the race test
        break;

    case WM_TIMER:
        if (pBuf) {
            // Check if anything changed (Seq number OR Counter value)
            if (pBuf->sequence_number != last_seq || pBuf->raceCounter != last_counter) {
                
                snprintf(buffer, sizeof(buffer), 
                    "Message: %s\n\n"
                    "Shared Counter: %d\n"
                    "(Target: 20 if 2 writers ran)", 
                    pBuf->message, pBuf->raceCounter);
                
                SetWindowText(hLabel, buffer);
                
                last_seq = pBuf->sequence_number;
                last_counter = pBuf->raceCounter;
            }
        }
        break;

    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR args, int nShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "ReaderClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    HWND hwnd = CreateWindow("ReaderClass", "IPC Reader (Observer)", WS_OVERLAPPEDWINDOW, 550, 100, 400, 200, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}