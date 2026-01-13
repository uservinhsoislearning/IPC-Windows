# Inter-process communication
This repository aims to simulate a shared-data communication system between 2 processes. In reality, this can happen on a system of computers (a supercomputer). This project used C++ as the main coding language.
## How to run?
You need to download MinGW first. Follow this link to download [MinGW](https://sourceforge.net/projects/mingw/)
There are 2 versions of the code in the repository, the first version is just a simple shared-memory message sender/receiver. The second version implements the lock method and the semaphore method to handle synchronization.

If you want to run the first version, create 2 terminals inside your working directory. Then change the working directory to version_1:
```
cd version_1
```

If you want to run the second version, create 3 terminals inside your working directory. Then change the working directory to version_2:
```
cd version_2
```

Afterwards, run all .exe files (for version 1: Reader.exe and Writer.exe, for version 2: Reader.exe, Writer1.exe, and Writer2.exe)

## Dependecies
All libraries used in this repository are supported by the standard library of C/C++. After downloading MinGW, you don't need to download any other libraries.
