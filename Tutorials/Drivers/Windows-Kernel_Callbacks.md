# Windows Kernel Callbacks

In this tutorial, we explore **callback routines in the Windows kernel**, explaining what they are, how they work, and how they can be used in drivers. This tutorial serves as an introduction to callback routines and uses **PsSetCreateProcessNotifyRoutine** as an implementation example.

### What Are Callback Routines?

**Callback routines** in the Windows kernel provide an efficient way for drivers to receive notifications when specific system events occur. They allow a driver to **listen for events** without continuously polling the system state, which can help improve performance and efficiency.

Some relevant callback and notification routines include:

1. **PsSetCreateProcessNotifyRoutineEx():** Notifies a driver when a process is created or exits.
2. **PsSetCreateThreadNotifyRoutine():** Notifies a driver when a thread is created or exits.
3. **PsSetLoadImageNotifyRoutine():** Notifies a driver when an image, such as a driver or DLL, is loaded.
4. **PsCreateSystemThread():** Creates a system thread in kernel mode; it is not a callback-registration routine.

### PsSetCreateProcessNotifyRoutine

The **PsSetCreateProcessNotifyRoutine** function allows the Windows kernel to notify a driver whenever a process is created or exits. This tutorial uses it as a practical example.

> **Note:** Check the current Microsoft documentation for supported usage, limits, and best practices before using process notification routines in production. See the [official documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreateprocessnotifyroutine).

### Step 1: Driver Setup

Before implementing the callback routine, we need to set up a basic driver.

```c
#include <ntddk.h>

// Global variables
PVOID g_ProcessNotifyHandle = NULL;

// Prototypes
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID UnloadDriver(_In_ PDRIVER_OBJECT DriverObject);
VOID PcreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    // Register the process creation notification routine
    NTSTATUS status = PsSetCreateProcessNotifyRoutine(PcreateProcessNotifyRoutine, FALSE);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to register the process notification routine.\n");
        return status;
    }

    // Set the driver's unload routine
    DriverObject->DriverUnload = UnloadDriver;

    return STATUS_SUCCESS;
}

VOID UnloadDriver(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    // Remove the process creation notification routine
    PsSetCreateProcessNotifyRoutine(PcreateProcessNotifyRoutine, TRUE);
    DbgPrint("Driver unloaded successfully.\n");
}
```

### Step 2: Implementing the Callback Routine

Now we implement `PcreateProcessNotifyRoutine`, which is called whenever a process is created or exits.

```c
void PcreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
    KdPrint(("Parent process: %llu\n", (unsigned long long)ParentId));
    KdPrint(("Process ID: %llu\n", (unsigned long long)ProcessId));
    if (Create)
    {
        KdPrint(("Process created\n"));
    }
    else
    {
        KdPrint(("Process exited\n"));
    }
}
```

### Conclusion

In this tutorial, we explored **callback routines in the Windows kernel** and how drivers can use them to monitor specific events, such as process creation and termination. We used **PsSetCreateProcessNotifyRoutine** as a practical example of a routine that notifies a driver when a process is created or exits.

This tutorial is an introduction to Windows kernel callbacks. Many other notification routines are available, including those for thread creation and image loading.
