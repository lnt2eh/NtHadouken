#include <ntddk.h>
/*
  Basic driver that prints Hello World
*/

VOID kernelUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("Unloading Driver\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    KdPrint(("Hello, World\n"));
    DriverObject->DriverUnload = kernelUnload;
    return STATUS_SUCCESS;
}
