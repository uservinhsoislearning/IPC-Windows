#include <windows.h>
#include <stdio.h>
#include "SharedData.h"

#define ID_LABEL_DISPLAY 1
#define TIMER_ID 999

HANDLE hMapFile;
SharedData* pBuf;
HWND hLabel;
int last_sequence = -1;

void Cleanup() {
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char displayBuffer[BUF_SIZE + 50];

    switch (msg) {
    case WM_CREATE:
        // 1. Create a Label to display data
        hLabel = CreateWindow("STATIC", "Waiting for data...", 
            WS_CHILD | WS_VISIBLE,
            20, 50, 400, 50, hwnd, (HMENU)ID_LABEL_DISPLAY, NULL, NULL);

        // 2. Open Existing Shared Memory
        hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,   // Read/Write access
            FALSE,                 // Do not inherit the name
            SHARED_MEM_NAME);      // Name of mapping object

        if (hMapFile == NULL) {
            MessageBox(hwnd, "Could not open file mapping object. Run Writer first!", "Error", MB_OK);
            return -1;
        }

        pBuf = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));

        if (pBuf == NULL) {
            MessageBox(hwnd, "Could not map view of file.", "Error", MB_OK);
            return -1;
        }

        // 3. Set a Timer to poll for data every 500ms
        SetTimer(hwnd, TIMER_ID, 500, NULL);
        break;

    case WM_TIMER:
        // Poll shared memory for changes
        if (pBuf != NULL && pBuf->sequence_number != last_sequence) {
            // READ FROM SHARED MEMORY
            snprintf(displayBuffer, sizeof(displayBuffer), 
                "Message: %s\nSeq: %d\nFrom Process ID: %lu", 
                pBuf->message, pBuf->sequence_number, pBuf->senderPid);
            
            // Update GUI
            SetWindowText(hLabel, displayBuffer);
            
            // Update local state
            last_sequence = pBuf->sequence_number;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        Cleanup();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "ReaderWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(CLASS_NAME, "IPC Reader (Client)", WS_OVERLAPPEDWINDOW,
        600, 100, 450, 250, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}