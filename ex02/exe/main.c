#include <Windows.h>
#include <stdio.h>

#define DRIVER_FUNC 0x01

#define IOCTL_XXX \
  CTL_CODE(FILE_DEVICE_UNKNOWN, DRIVER_FUNC, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PROCESS_IMAGE_PATH_ENTRY {
  DWORD nextOffset;  // if 0, current is the last entry.
  WCHAR path[ANYSIZE_ARRAY];
} PROCESS_IMAGE_PATH_ENTRY, *PPROCESS_IMAGE_PATH_ENTRY;

PPROCESS_IMAGE_PATH_ENTRY InitializeProcessImagePathEntries(
    LPCWSTR pathList[],
    DWORD numberOfEntries) {
  if (numberOfEntries == 0) {
    return NULL;
  }

  size_t totalSize = 0;
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
                              ? 0
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

int main() {
  /*
   * For simplistic, the following list is consist of hard-coded string.
   * In general, we should've read this from the file such as policy.
   */
  LPCWSTR imagePathList[3] = {
      L"C:\\Windows\\notepad.exe",
      L"C:\\Windows\\System32\\calc.exe",
      L"C:\\Windows\\SysWOW64\\calc.exe",
  };

  PPROCESS_IMAGE_PATH_ENTRY baseEntry = InitializeProcessImagePathEntries(
      imagePathList, sizeof(imagePathList) / sizeof(LPCWSTR));
  if (baseEntry == NULL) {
    return 1;
  }

  PPROCESS_IMAGE_PATH_ENTRY entries = baseEntry;

  while (TRUE) {
    printf("%ws\n", entries->path);

    if (entries->nextOffset == 0) {
      break;
    }

    entries = (PPROCESS_IMAGE_PATH_ENTRY)((PBYTE)entries + entries->nextOffset);
  }

  FinalizeProcessImagePathEntries(baseEntry);

  return 0;
}