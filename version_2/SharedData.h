#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <windows.h>

#define SHARED_MEM_NAME "Local\\MySharedMemoryProject"
#define SEMAPHORE_NAME  "Local\\MyIPCSemaphore"
#define BUF_SIZE 256

struct SharedData {
    // Standard Message Data
    char message[BUF_SIZE];
    int sequence_number;

    // Race Condition Test Data
    volatile LONG spinLock; // Method 1: Shared Variable Lock
    volatile int raceCounter; // The counter we will try to break
};

#endif