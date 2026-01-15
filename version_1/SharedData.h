#ifndef SHARED_DATA_H
#define SHARED_DATA_H

// The unique name for our shared memory object
#define SHARED_MEM_NAME "Local\\MySharedMemoryProject"

// The size of the buffer
#define BUF_SIZE 256

// The structure we will store in shared memory
struct SharedData {
    char message[BUF_SIZE];
    int sequence_number; // Used to detect if data has changed
    unsigned long senderPid;
};

#endif