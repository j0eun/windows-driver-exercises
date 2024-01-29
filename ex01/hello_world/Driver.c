#include <ntddk.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
                     _In_ PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(DriverObject);
  UNREFERENCED_PARAMETER(RegistryPath);

  DbgPrint("Hello, world!\n");

  return STATUS_UNSUCCESSFUL;  // intended driver unload
}
