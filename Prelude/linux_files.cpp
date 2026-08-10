#include "linux_aux_wrapper.h"
#if defined(__linux__) || defined(__APPLE__)
#include <stdio.h>

HANDLE CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void * lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {

	const char * mode;
	if (dwDesiredAccess == GENERIC_READ) {
		mode = "r";
	}
	else {
		printf("unimplemented CreateFile\n");
		exit(0);
	}
	
	FILE * fp = fopen(lpFileName,mode);
	if (!fp) {
		return (HANDLE)-1;
	}
	return fp;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void * lpOverlapped) {
	FILE * fp = (FILE*)hFile;
	int rt = fread(lpBuffer, 1, nNumberOfBytesToRead, fp);
	*lpNumberOfBytesRead = rt;
	return 1;
}


// only for files for now
BOOL CloseHandle(HANDLE hObject) {
	fclose((FILE*)hObject);
	return TRUE;
}

#endif