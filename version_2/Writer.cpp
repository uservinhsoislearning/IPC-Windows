#include <windows.h>
#include <stdio.h>
#include "SharedData.h"

// ID for controls
#define ID_EDIT_BOX 1
#define ID_BTN_SEND 2
#define ID_BTN_RACE 3
#define ID_RB_NONE  4
#define ID_RB_SPIN  5
#define ID_RB_SEM   6

HANDLE hMapFile;
HANDLE hSemaphore; // Kernel Object for Method 2
SharedData* pBuf;
HWND hEdit;

// Locking Mode State
int currentLockMode = ID_RB_NONE;

void Cleanup() {
    if (pBuf) UnmapViewOfFile(pBuf);
    if (hMapFile) CloseHandle(hMapFile);
    if (hSemaphore) CloseHandle(hSemaphore);
}

// --- SYNCHRONIZATION HELPERS ---

void AcquireLock() {
    if (currentLockMode == ID_RB_NONE) {
        return; // No protection
    }
    else if (currentLockMode == ID_RB_SPIN) {
        // Method 1: Spinlock using Atomic shared variable
        // Try to set spinLock to 1. If it was already 1, wait and try again.
        while (InterlockedCompareExchange(&pBuf->spinLock, 1, 0) != 0) {
            Sleep(0); // Yield processor to other threads/processes
        }
    }
    else if (currentLockMode == ID_RB_SEM) {
        // Method 2: System Semaphore
        WaitForSingleObject(hSemaphore, INFINITE);
    }
}

void ReleaseLock() {
    if (currentLockMode == ID_RB_NONE) {
        return;
    }
    else if (currentLockMode == ID_RB_SPIN) {
        // Method 1: Reset variable to 0
        pBuf->spinLock = 0; // Simple assignment is atomic for aligned 32-bit integers
    }
    else if (currentLockMode == ID_RB_SEM) {
        // Method 2: Release Semaphore
        ReleaseSemaphore(hSemaphore, 1, NULL);
    }
}

// --- RACE CONDITION TEST ---
// Increments the counter 10,000 times
// --- RACE CONDITION TEST (Slow Version) ---
// Increments the counter 10 times, taking ~10 seconds
DWORD WINAPI RaceThread(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    char debugBuf[64];

    for (int i = 0; i < 10; i++) {
        AcquireLock();

        // [CRITICAL SECTION START]
        int temp = pBuf->raceCounter;

        // Force a Race Condition:
        // We simulate "heavy work" or network latency here.
        // While Process A sleeps here holding the value '0', 
        // Process B will also read '0' (if no lock is used).
        Sleep(1000); // 1 Second delay per increment

        pBuf->raceCounter = temp + 1;
        // [CRITICAL SECTION END]

        ReleaseLock();
        
        // Optional: Update GUI to show progress
        char progress[32];
        snprintf(progress, 32, "Writing... %d/10", i+1);
        SetWindowText(hEdit, progress);
    }

    snprintf(debugBuf, sizeof(debugBuf), "Finished 10 increments.\nCurrent Count: %d", pBuf->raceCounter);
    MessageBox(hwnd, debugBuf, "Thread Done", MB_OK);

    SetWindowText(hEdit, "Done.");
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // 1. GUI Setup
        CreateWindow("BUTTON", "Start Race Test (write 10)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 50, 200, 30, hwnd, (HMENU)ID_BTN_RACE, NULL, NULL);

        // Radio Buttons for Lock Type
        CreateWindow("BUTTON", "No Lock (Fail)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 10, 90, 150, 20, hwnd, (HMENU)ID_RB_NONE, NULL, NULL);
        CreateWindow("BUTTON", "Method 1: Spinlock", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 10, 115, 150, 20, hwnd, (HMENU)ID_RB_SPIN, NULL, NULL);
        CreateWindow("BUTTON", "Method 2: Semaphore", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 10, 140, 150, 20, hwnd, (HMENU)ID_RB_SEM, NULL, NULL);
        
        // Select "No Lock" by default
        CheckRadioButton(hwnd, ID_RB_NONE, ID_RB_SEM, ID_RB_NONE);

        // 2. Init Shared Memory
        hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), SHARED_MEM_NAME);
        if (hMapFile == NULL) return -1;
        pBuf = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
        if (pBuf == NULL) return -1;

        // Init Data safely (only if we are the first creator, detected by GetLastError)
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            pBuf->sequence_number = 0;
            pBuf->spinLock = 0;
            pBuf->raceCounter = 0;
            memset(pBuf->message, 0, BUF_SIZE);
        }

        // 3. Init Semaphore (Method 2)
        // Initial count = 1, Max count = 1. This makes it a Mutex behavior.
        hSemaphore = CreateSemaphore(NULL, 1, 1, SEMAPHORE_NAME);

        break;

    case WM_COMMAND:
        // Handle Radio Button Changes
        if (LOWORD(wParam) >= ID_RB_NONE && LOWORD(wParam) <= ID_RB_SEM) {
            currentLockMode = LOWORD(wParam);
        }

        // Handle Send Message
        if (LOWORD(wParam) == ID_BTN_SEND) {
            AcquireLock(); 
            char temp[BUF_SIZE];
            GetWindowText(hEdit, temp, BUF_SIZE);
            strncpy(pBuf->message, temp, BUF_SIZE);
            pBuf->message[BUF_SIZE-1] = '\0'; // Safety
            pBuf->sequence_number++;
            ReleaseLock();
        }

        // Handle Race Test
        if (LOWORD(wParam) == ID_BTN_RACE) {
            // We create a thread so the GUI doesn't freeze
            CreateThread(NULL, 0, RaceThread, hwnd, 0, NULL);
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
    wc.lpszClassName = "WriterClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    HWND hwnd = CreateWindow("WriterClass", "IPC Writer (Race Tester)", WS_OVERLAPPEDWINDOW, 100, 100, 420, 250, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}