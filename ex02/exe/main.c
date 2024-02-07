#include <Windows.h>
#include <stdio.h>

#define IOCTL_SET_PROCESS_BLACKLIST \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PROCESS_IMAGE_PATH_ENTRY {
  DWORD nextOffset;
  WCHAR path[ANYSIZE_ARRAY];
} PROCESS_IMAGE_PATH_ENTRY, *PPROCESS_IMAGE_PATH_ENTRY;

PPROCESS_IMAGE_PATH_ENTRY InitializeProcessImagePathEntries(
    LPCWSTR pathList[],
    DWORD numberOfEntries) {
  if (numberOfEntries == 0) {
    return NULL;
  }

  DWORD totalSize = 0;
  for (DWORD i = 0; i < numberOfEntries; i++) {
    /*
     * The FIELD_OFFSET macro is useful when calculating size of a structure
     * that contain a variadic string.
     */
    totalSize +=
        FIELD_OFFSET(PROCESS_IMAGE_PATH_ENTRY, path[wcslen(pathList[i]) + 1]);
  }

  PPROCESS_IMAGE_PATH_ENTRY baseEntry =
      (PPROCESS_IMAGE_PATH_ENTRY)malloc(totalSize);
  if (baseEntry == NULL) {
    return NULL;
  }

  PPROCESS_IMAGE_PATH_ENTRY entries = baseEntry;

  ZeroMemory(entries, totalSize);

  for (DWORD i = 0; i < numberOfEntries; i++) {
    entries->nextOffset = (i + 1 >= numberOfEntries)
                              ? 0  // if 0, current is the last entry.
                              : FIELD_OFFSET(PROCESS_IMAGE_PATH_ENTRY,
                                             path[wcslen(pathList[i]) + 1]);
    memcpy(entries->path, pathList[i],
           (wcslen(pathList[i]) + 1) * sizeof(WCHAR));

    entries = (PPROCESS_IMAGE_PATH_ENTRY)((PBYTE)entries + entries->nextOffset);
  }

  return baseEntry;
}

void FinalizeProcessImagePathEntries(PPROCESS_IMAGE_PATH_ENTRY entries) {
  free(entries);
  entries = NULL;

  return;
}

DWORD GetSizeOfProcessImagePathEntries(PPROCESS_IMAGE_PATH_ENTRY entries) {
  DWORD totalSize = 0;

  while (TRUE) {
    totalSize +=
        FIELD_OFFSET(PROCESS_IMAGE_PATH_ENTRY, path[wcslen(entries->path) + 1]);

    if (entries->nextOffset == 0) {
      break;
    }

    entries = (PPROCESS_IMAGE_PATH_ENTRY)((PBYTE)entries +
                                          FIELD_OFFSET(
                                              PROCESS_IMAGE_PATH_ENTRY,
                                              path[wcslen(entries->path) + 1]));
  }

  return totalSize;
}

int main() {
  HANDLE hDriver = CreateFile(TEXT("\\\\.\\Ex02"), GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDriver == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "CreateFile failed : %d\n", GetLastError());
    return 1;
  }

  /*
   * For simplistic, the following list is consist of hard-coded string.
   * In general, we should've read this from the file such as policy.
   */
  LPCWSTR imagePathList[3] = {
      L"C:\\Windows\\notepad.exe",
      L"C:\\Windows\\System32\\calc.exe",
      L"C:\\Windows\\SysWOW64\\calc.exe",
  };

  PPROCESS_IMAGE_PATH_ENTRY entries = InitializeProcessImagePathEntries(
      imagePathList, sizeof(imagePathList) / sizeof(LPCWSTR));
  if (entries == NULL) {
    return 1;
  }

  DWORD bytesReturned = 0;
  if (DeviceIoControl(hDriver, IOCTL_SET_PROCESS_BLACKLIST, entries,
                      GetSizeOfProcessImagePathEntries(entries), NULL, 0,
                      &bytesReturned, NULL) == 0) {
    fprintf(stderr, "DeviceIoControl failed : %d\n", GetLastError());
    return 1;
  }

  FinalizeProcessImagePathEntries(entries);

  CloseHandle(hDriver);

  return 0;
}