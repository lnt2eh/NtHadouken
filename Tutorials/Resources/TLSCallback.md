# Thread Local Storage (TLS) Callbacks

This article introduces the Windows Thread Local Storage (TLS) feature and the TLS callback mechanism. It explains what TLS is, how it works, and provides a practical code example for readers who want to experiment and make their own modifications. This article will also serve as a foundation for a future NtHadouken post about Windows anti-reversing techniques.

## What Is Thread Local Storage (TLS)?

A function's local variables are private to each thread executing that function. However, static and global variables are shared by all threads in a process. It is worth noting that all threads in a process share the same virtual address space.

TLS makes it possible to provide data that is private to each thread and can be accessed by the process through a global index. A thread allocates the index, which can then be used by threads to retrieve the thread-specific data associated with it.

## How Does It Work?

When threads are created, the system provides a TLS array of values (**LPVOID**), which are initially set to **NULL**. Before an index can be used, it must be allocated by a thread.

Each thread stores its data for a TLS index in its own array.

## TLS Callback

TLS callbacks are a Windows mechanism that allows a program to register a function that is called during process or thread lifecycle events. These callbacks can be used for several purposes, such as initializing thread-specific data or performing other setup tasks.

TLS callbacks have also been used by malware authors as an anti-debugging technique. A program can use a TLS callback to execute code before the main entry point defined in the PE image. For example, a program could check whether it is being debugged and, if so, terminate the process or take other measures to hinder analysis.

This topic will be explored further in the [Reverse Engineering](https://github.com/lnt2eh/NtHadouken/tree/main/Tutorials/Reverse-Engineering) section.

## Practical Example

```c
/*
 * Practical example for experimentation.
 * Created by _int2Eh for the NtHadouken project.
 */

#include <windows.h>
#include <stdio.h>

/*
 * TLS callback prototype.
 * This function is invoked automatically during process/thread lifecycle events.
 */
void NTAPI TLSCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved);

/*
 * Define the callback pointer in the .CRT$XLB section.
 * The Windows loader uses this section to locate TLS callbacks.
 */
#ifdef _MSC_VER
// MSVC
#pragma const_seg(".CRT$XLB")
EXTERN_C const PIMAGE_TLS_CALLBACK pTLSCallback = TLSCallback;
#pragma const_seg()
#else
// MinGW
PIMAGE_TLS_CALLBACK pTLSCallback __attribute__((section(".CRT$XLB"))) = TLSCallback;
#endif

/*
 * TLS callback implementation.
 * Code here can run before the program's main entry point (main or WinMain).
 */
void NTAPI TLSCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    switch (Reason) {
        case DLL_PROCESS_ATTACH:
            /*
             * Check whether the process is being debugged.
             * This can be used as an anti-debugging technique.
             */
            if (IsDebuggerPresent()) {
                MessageBoxA(NULL, "Debugger detected! Terminating the process...", "TLS Alert", MB_ICONERROR);
                ExitProcess(1);
            } else {
                MessageBoxA(NULL, "TLS Callback: DLL_PROCESS_ATTACH without a debugger.", "TLS Info", MB_OK);
            }
            break;
        case DLL_THREAD_ATTACH:
            MessageBoxA(NULL, "TLS Callback: DLL_THREAD_ATTACH", "TLS Info", MB_OK);
            break;
        case DLL_THREAD_DETACH:
            MessageBoxA(NULL, "TLS Callback: DLL_THREAD_DETACH", "TLS Info", MB_OK);
            break;
        case DLL_PROCESS_DETACH:
            MessageBoxA(NULL, "TLS Callback: DLL_PROCESS_DETACH", "TLS Info", MB_OK);
            break;
    }
}

/*
 * Program entry point.
 * This code runs only if the TLS callback does not detect a debugger and terminate the process.
 */
int main() {
    MessageBoxA(NULL, "Main Entry Point (main)", "NtHadouken", MB_OK);
    return 0;
}
```
