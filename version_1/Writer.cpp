#include <windows.h>
#include <stdio.h>
#include "SharedData.h"

// ID for the GUI controls
#define ID_EDIT_BOX 1
#define ID_BTN_SEND 2

// Global handles
HANDLE hMapFile;
SharedData* pBuf;
HWND hEdit; 

// Function to clean up resources
void Cleanup() {
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
}

// Window Procedure: Handles events (clicks, painting, etc.)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // 1. Create the Edit Box (Input)
        hEdit = CreateWindow("EDIT", "", 
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            50, 50, 300, 25, hwnd, (HMENU)ID_EDIT_BOX, NULL, NULL);

        // 2. Create the Send Button
        CreateWindow("BUTTON", "Send to Shared Memory", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 90, 200, 30, hwnd, (HMENU)ID_BTN_SEND, NULL, NULL);

        // 3. Initialize Shared Memory
        hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // Use paging file
            NULL,                    // Default security
            PAGE_READWRITE,          // Read/Write permission
            0,                       // Max. object size (high-order DWORD)
            sizeof(SharedData),      // Max. object size (low-order DWORD)
            SHARED_MEM_NAME);        // Name of mapping object

        if (hMapFile == NULL) {
            MessageBox(hwnd, "Could not create file mapping object.", "Error", MB_OK);
            return -1;
        }

        pBuf = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
        
        if (pBuf == NULL) {
            MessageBox(hwnd, "Could not map view of file.", "Error", MB_OK);
            return -1;
        }

        // Initialize memory
        pBuf->sequence_number = 0;
        memset(pBuf->message, 0, BUF_SIZE);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_SEND) {
            // Get text from Edit box
            char tempBuffer[BUF_SIZE];
            GetWindowText(hEdit, tempBuffer, BUF_SIZE);

            // WRITE TO SHARED MEMORY
            strncpy(pBuf->message, tempBuffer, BUF_SIZE);
            pBuf->message[BUF_SIZE - 1] = '\0';
            pBuf->sequence_number++; // Increment to signal new data

            MessageBox(hwnd, "Data written to Shared Memory!", "Success", MB_OK);
        }
        break;

    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Main Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "WriterWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(CLASS_NAME, "IPC Writer (Server)", WS_OVERLAPPEDWINDOW,
        100, 100, 450, 250, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}