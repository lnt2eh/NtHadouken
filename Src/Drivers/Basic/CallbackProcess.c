#include <ntddk.h>
/*
  This driver is an example of using the PsSetCreateProcessNotifyRoutine routine.
  Note that using this function for production purposes is no longer recommended.
*/

VOID kernelUnload(_In_ PDRIVER_OBJECT DriverObject);
VOID PcreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create);

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status = PsSetCreateProcessNotifyRoutine(PcreateProcessNotifyRoutine, FALSE);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to register the process notification routine.\n");
        return status;
    }

    DriverObject->DriverUnload = kernelUnload;
    return STATUS_SUCCESS;
}

VOID kernelUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    PsSetCreateProcessNotifyRoutine(PcreateProcessNotifyRoutine, TRUE);
    DbgPrint("Driver unloaded successfully.\n");
}

void PcreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
    KdPrint(("ProcessParent: %llu\n", (unsigned long long)ParentId));
    KdPrint(("ProcessId: %llu\n", (unsigned long long)ProcessId));
    if (Create)
    {
        KdPrint(("Process created\n"));
    }
    else
    {
        KdPrint(("Process terminated\n"));
    }
}
