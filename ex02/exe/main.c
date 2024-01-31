#include <Windows.h>
#include <stdio.h>

typedef struct _PROCESS_IMAGE_PATH_ENTRY {
  DWORD nextOffset;  // if 0, current is the last entry.
  WCHAR path[ANYSIZE_ARRAY];
} PROCESS_IMAGE_PATH_ENTRY, *PPROCESS_IMAGE_PATH_ENTRY;

PPROCESS_IMAGE_PATH_ENTRY InitProcessImagePathEntries(LPCWSTR pathList[],
                                                      DWORD numberOfEntries) {
  if (numberOfEntries == 0) {
    return NULL;
  }

  size_t totalSize = 0;
  PDWORD nextOffsets = (PDWORD)malloc(numberOfEntries * sizeof(DWORD));

  if (!nextOffsets) {
    return NULL;
  }

  ZeroMemory(nextOffsets, numberOfEntries * sizeof(DWORD));

  for (DWORD i = 0; i < numberOfEntries; i++) {
    nextOffsets[i] = (wcslen(pathList[i]) + 1) * sizeof(WCHAR) +
                     sizeof(PROCESS_IMAGE_PATH_ENTRY) - sizeof(WCHAR);
    totalSize += nextOffsets[i];
  }

  PPROCESS_IMAGE_PATH_ENTRY baseEntry =
      (PPROCESS_IMAGE_PATH_ENTRY)malloc(totalSize);

  if (!baseEntry) {
    free(nextOffsets);
    return NULL;
  }

  PPROCESS_IMAGE_PATH_ENTRY entries = baseEntry;

  ZeroMemory(entries, totalSize);

  for (DWORD i = 0; i < numberOfEntries; i++) {
    entries->nextOffset = (i + 1 >= numberOfEntries) ? 0 : nextOffsets[i];
    memcpy(entries->path, pathList[i],
           (wcslen(pathList[i]) + 1) * sizeof(WCHAR));

    entries = (PPROCESS_IMAGE_PATH_ENTRY)((PBYTE)entries + entries->nextOffset);
  }

  free(nextOffsets);

  return baseEntry;
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

  PPROCESS_IMAGE_PATH_ENTRY baseEntry = InitProcessImagePathEntries(
      imagePathList, sizeof(imagePathList) / sizeof(LPCWSTR));
  if (!baseEntry) {
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

  free(baseEntry);

  return 0;
}