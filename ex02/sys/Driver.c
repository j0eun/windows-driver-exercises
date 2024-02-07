#include <ntddk.h>

#define NT_DEVICE_NAME L"\\Device\\Ex02"
#define DOS_DEVICE_NAME L"\\DosDevices\\Ex02"

#define IOCTL_SET_PROCESS_BLACKLIST \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PROCESS_IMAGE_PATH_ENTRY {
  ULONG nextOffset;
  WCHAR path[ANYSIZE_ARRAY];
} PROCESS_IMAGE_PATH_ENTRY, *PPROCESS_IMAGE_PATH_ENTRY;

NTSTATUS IrpMajorCreateCallback(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PAGED_CODE();

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS IrpMajorCloseCallback(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PAGED_CODE();

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

VOID PrintProcessImagePathEntries(_In_ PPROCESS_IMAGE_PATH_ENTRY entries) {
  while (TRUE) {
    DbgPrint("%ws\n", entries->path);

    if (entries->nextOffset == 0) {
      break;
    }

    entries = (PPROCESS_IMAGE_PATH_ENTRY)((PCHAR)entries +
                                          FIELD_OFFSET(
                                              PROCESS_IMAGE_PATH_ENTRY,
                                              path[wcslen(entries->path) + 1]));
  }
}

NTSTATUS IrpMajorDeviceControlCallback(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PAGED_CODE();

  NTSTATUS ResponseStatusCode = STATUS_SUCCESS;
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  ULONG InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SET_PROCESS_BLACKLIST:
      if (InputBufferLength == 0) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
      }

      PPROCESS_IMAGE_PATH_ENTRY ProcessBlackList =
          Irp->AssociatedIrp.SystemBuffer;
      PrintProcessImagePathEntries(ProcessBlackList);

      ResponseStatusCode = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;
      break;

    default:
      ResponseStatusCode = STATUS_INVALID_DEVICE_REQUEST;
      break;
  }

  Irp->IoStatus.Status = ResponseStatusCode;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return ResponseStatusCode;
}

VOID UnloadDriver(_In_ PDRIVER_OBJECT DriverObject) {
  PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
  UNICODE_STRING Win32DeviceName;

  PAGED_CODE();

  RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);

  IoDeleteSymbolicLink(&Win32DeviceName);

  if (DeviceObject != NULL) {
    IoDeleteDevice(DeviceObject);
  }
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
                     _In_ PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  UNICODE_STRING NtUnicodeString;
  UNICODE_STRING NtWin32NameString;

  RtlInitUnicodeString(&NtUnicodeString, NT_DEVICE_NAME);

  NTSTATUS NtStatus = IoCreateDevice(
      DriverObject, 0, &NtUnicodeString, FILE_DEVICE_UNKNOWN,
      FILE_DEVICE_SECURE_OPEN, FALSE, &DriverObject->DeviceObject);

  if (!NT_SUCCESS(NtStatus)) {
    return NtStatus;
  }

  DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpMajorCreateCallback;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpMajorCloseCallback;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
      IrpMajorDeviceControlCallback;
  DriverObject->DriverUnload = UnloadDriver;

  RtlInitUnicodeString(&NtWin32NameString, DOS_DEVICE_NAME);

  NtStatus = IoCreateSymbolicLink(&NtWin32NameString, &NtUnicodeString);

  if (!NT_SUCCESS(NtStatus)) {
    IoDeleteDevice(DriverObject->DeviceObject);
  }

  return NtStatus;
}
