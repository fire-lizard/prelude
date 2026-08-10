#ifndef _LINUX_WRAPPER_
#define _LINUX_WRAPPER_

#if defined(__linux__) || defined(__APPLE__)

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef int32_t LONG;
typedef uint32_t HRESULT;
typedef void * LPVOID;
typedef unsigned short int WORD, *LPWORD;
typedef void * HWND, *HANDLE, *HDC, *LPUNKNOWN, *HINSTANCE;
typedef unsigned int DWORD, *LPDWORD;
typedef unsigned char BYTE, *LPBYTE;
typedef unsigned int UINT;
typedef uint32_t ULONG;
typedef int BOOL;
typedef void VOID;
typedef unsigned char UCHAR;

typedef char CHAR;


typedef char* LPSTR;
typedef const char *LPCSTR;

typedef struct tchar {

} tchar, TCHAR;

typedef struct _HMMIO {

} *HMMIO;



typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

typedef struct _RGNDATAHEADER {
	DWORD dwSize;
	DWORD iType;
	DWORD nCount;
	DWORD nRgnSize;
	RECT  rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct tagLOGFONT {
	LONG lfHeight;
	LONG lfWidth;
	LONG lfEscapement;
	LONG lfOrientation;
	LONG lfWeight;
	BYTE lfItalic;
	BYTE lfUnderline;
	BYTE lfStrikeOut;
	BYTE lfCharSet;
	BYTE lfOutPrecision;
	BYTE lfClipPrecision;
	BYTE lfQuality;
	BYTE lfPitchAndFamily;
	CHAR lfFaceName[32];
} LOGFONT, *PLOGFONT, *NPLOGFONT, *LPLOGFONT;



typedef struct _RGNDATA {
	RGNDATAHEADER rdh;
	char          Buffer[1];
} RGNDATA, *PRGNDATA, *NPRGNDATA, *LPRGNDATA;

typedef struct IUnknown {

} IUnknown;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

typedef struct tagTEXTMETRIC {
	LONG tmHeight;
	LONG tmAscent;
	LONG tmDescent;
	LONG tmInternalLeading;
	LONG tmExternalLeading;
	LONG tmAveCharWidth;
	LONG tmMaxCharWidth;
	LONG tmWeight;
	LONG tmOverhang;
	LONG tmDigitizedAspectX;
	LONG tmDigitizedAspectY;
	BYTE tmFirstChar;
	BYTE tmLastChar;
	BYTE tmDefaultChar;
	BYTE tmBreakChar;
	BYTE tmItalic;
	BYTE tmUnderlined;
	BYTE tmStruckOut;
	BYTE tmPitchAndFamily;
	BYTE tmCharSet;
} TEXTMETRIC, *PTEXTMETRIC, *NPTEXTMETRIC, *LPTEXTMETRIC;

typedef struct _ABC {
	int  abcA;
	UINT abcB;
	int  abcC;
} ABC, *PABC, *NPABC, *LPABC;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD          bmiColors[1];
} __attribute__((packed)) BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;

#define __stdcall
#define FAR
#define TRUE 1
#define FALSE 0

#define CALLBACK
#define WINAPI



// sound

typedef struct waveformat_tag {
	WORD    wFormatTag;        /* format type */
	WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
	DWORD   nSamplesPerSec;    /* sample rate */
	DWORD   nAvgBytesPerSec;   /* for buffer estimation */
	WORD    nBlockAlign;       /* block size of data */
} WAVEFORMAT, *PWAVEFORMAT, *NPWAVEFORMAT, *LPWAVEFORMAT;

typedef struct pcmwaveformat_tag {
	WAVEFORMAT  wf;
	WORD        wBitsPerSample;
} PCMWAVEFORMAT, *PPCMWAVEFORMAT, *NPPCMWAVEFORMAT, *LPPCMWAVEFORMAT;

// hw memory mapping 

typedef struct _MMCKINFO {
	DWORD          ckid;           /* chunk ID */
	DWORD           cksize;         /* chunk size */
	DWORD          fccType;        /* form type or list type */
	DWORD           dwDataOffset;   /* offset of data portion of chunk */
	DWORD           dwFlags;        /* flags used by MMIO functions */
} MMCKINFO, *PMMCKINFO, *NPMMCKINFO, *LPMMCKINFO;

typedef struct _MMIOINFO {

} MMIOINFO, *PMMIOINFO, *NPMMIOINFO, *LPMMIOINFO;
typedef const MMIOINFO *LPCMMIOINFO;

UINT mmioDescend(HMMIO hmmio,LPMMCKINFO pmmcki,const MMCKINFO FAR * pmmckiParent,UINT fuDescend);
LONG mmioRead(HMMIO hmmio,char * pch,LONG cch);
UINT mmioAscend(HMMIO hmmio,LPMMCKINFO pmmcki,UINT fuAscend);
HMMIO mmioOpen(LPSTR pszFileName,LPMMIOINFO pmmioinfo,DWORD fdwOpen);
DWORD mmioClose(HMMIO hmmio, UINT fuClose);

#define E_FAIL                           0x80004005L
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)
#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST     mmioFOURCC('L', 'I', 'S', 'T')

/* various MMIO flags */
#define MMIO_FHOPEN             0x0010  /* mmioClose: keep file handle open */
#define MMIO_EMPTYBUF           0x0010  /* mmioFlush: empty the I/O buffer */
#define MMIO_TOUPPER            0x0010  /* mmioStringToFOURCC: to u-case */
#define MMIO_INSTALLPROC    0x00010000  /* mmioInstallIOProc: install MMIOProc */
#define MMIO_GLOBALPROC     0x10000000  /* mmioInstallIOProc: install globally */
#define MMIO_REMOVEPROC     0x00020000  /* mmioInstallIOProc: remove MMIOProc */
#define MMIO_UNICODEPROC    0x01000000  /* mmioInstallIOProc: Unicode MMIOProc */
#define MMIO_FINDPROC       0x00040000  /* mmioInstallIOProc: find an MMIOProc */
#define MMIO_FINDCHUNK          0x0010  /* mmioDescend: find a chunk by ID */
#define MMIO_FINDRIFF           0x0020  /* mmioDescend: find a LIST chunk */
#define MMIO_FINDLIST           0x0040  /* mmioDescend: find a RIFF chunk */
#define MMIO_CREATERIFF         0x0020  /* mmioCreateChunk: make a LIST chunk */
#define MMIO_CREATELIST         0x0040  /* mmioCreateChunk: make a RIFF chunk */

#define MMIO_EXIST      0x00004000      /* checks for existence of file */
#define MMIO_ALLOCBUF   0x00010000      /* mmioOpen() should allocate a buffer */
#define MMIO_GETTEMP    0x00020000      /* mmioOpen() should retrieve temp name */

#define MMIO_DIRTY      0x10000000      /* I/O buffer is dirty */

/* read/write mode numbers (bit field MMIO_RWMODE) */
#define MMIO_READ       0x00000000      /* open file for reading only */
#define MMIO_WRITE      0x00000001      /* open file for writing only */
#define MMIO_READWRITE  0x00000002      /* open file for reading and writing */

#define WAVE_FORMAT_PCM     1
#define S_OK     0

typedef char * HPSTR;

typedef struct _GUID {
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
} GUID;

typedef struct HKEY {

} HKEY, *PHKEY;

// FAILED here is just < 0 
#define FAILED(hr) (((DWORD)hr) < 0)
#define SUCCEEDED(hr) (((DWORD)(hr)) >= 0)

typedef DWORD DWORD_PTR;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

// threading
typedef DWORD(WINAPI *PTHREAD_START_ROUTINE)(
	LPVOID lpThreadParameter
	);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;
typedef DWORD SIZE_T, *PSIZE_T;
HANDLE CreateThread(void * lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
BOOL TerminateThread(HANDLE hThread,DWORD  dwExitCode);
HANDLE CreateMutex(void * lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
BOOL ReleaseMutex(HANDLE hMutex);
HANDLE CreateEvent(void * lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
DWORD WaitForSingleObject(HANDLE hHandle,DWORD dwMilliseconds);
BOOL SetEvent(HANDLE hEvent);

#define WAIT_OBJECT_0 0

#define WAIT_TIMEOUT 0x102

// misc
void Sleep(DWORD millis);
DWORD SuspendThread(HANDLE t);
DWORD ResumeThread(HANDLE t);
DWORD timeGetTime();


#define ZeroMemory(a,b) memset(a,0,b)

#define GetCurrentDirectory(a,b) getcwd(b,a)

#define RGB(r,g,b) ((DWORD)(r+(g<<8)+(b<<16)))
#define TextOut(...)


// files

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_WRITE                0x00000002  
#define FILE_SHARE_DELETE               0x00000004  

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_ATTRIBUTE_NORMAL               0x00000080 
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define INVALID_HANDLE_VALUE ((HANDLE)(DWORD)-1)

HANDLE CreateFile(LPCSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode, void * lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,void * lpOverlapped);
BOOL CloseHandle(HANDLE hObject);

// gdi

typedef struct tagBITMAPFILEHEADER {
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAP {
	LONG        bmType;
	LONG        bmWidth;
	LONG        bmHeight;
	LONG        bmWidthBytes;
	WORD        bmPlanes;
	WORD        bmBitsPixel;
	LPVOID      bmBits;
} BITMAP, *PBITMAP, *NPBITMAP, *LPBITMAP;

typedef struct tagPOINT {
	LONG  x;
	LONG  y;
} POINT, *PPOINT, *NPPOINT, *LPPOINT;

typedef struct _HBITMAP {

} *HBITMAP;

#define DIB_RGB_COLORS      0 /* color table in RGBs */
#define DIB_PAL_COLORS      1 /* color table in palette indices */
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */

int StretchDIBits(HDC hdc, int xDest,  int yDest,  int DestWidth,  int DestHeight,  int xSrc,  int ySrc,  int SrcWidth,  int SrcHeight,
	void * lpBits,  BITMAPINFO * lpbmi,  UINT iUsage,  DWORD rop);
int MessageBox(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);
void ShowCursor(BOOL bShow);
int GetObject(HANDLE h, int c, LPVOID pv);
HANDLE LoadImage(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad);
HDC CreateCompatibleDC(HDC h);
void * SelectObject(HDC hdc, void* h);
BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy,  HDC hdcSrc,  int x1,  int y1,  DWORD rop);
BOOL DeleteDC(HDC hdc);
BOOL DeleteObject(void* ho);
int GetSystemMetrics(int nIndex);
BOOL GetWindowRect(HWND hWnd,LPRECT lpRect);
BOOL DestroyWindow(HWND hWnd);
BOOL GetCursorPos(LPPOINT lpPoint);
BOOL SetCurrentDirectory(LPCSTR lpPathName);

#define RDH_RECTANGLES  1


#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#if(WINVER >= 0x0500)
#define MB_CANCELTRYCONTINUE        0x00000006L
#endif /* WINVER >= 0x0500 */


#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L
#define MB_ICONSTOP                 MB_ICONHAND

#define IMAGE_BITMAP        0
#define IMAGE_ICON          1
#define IMAGE_CURSOR        2

#define LR_DEFAULTCOLOR     0x00000000
#define LR_MONOCHROME       0x00000001
#define LR_COLOR            0x00000002
#define LR_COPYRETURNORG    0x00000004
#define LR_COPYDELETEORG    0x00000008
#define LR_LOADFROMFILE     0x00000010
#define LR_LOADTRANSPARENT  0x00000020
#define LR_DEFAULTSIZE      0x00000040
#define LR_VGACOLOR         0x00000080
#define LR_LOADMAP3DCOLORS  0x00001000
#define LR_CREATEDIBSECTION 0x00002000
#define LR_COPYFROMRESOURCE 0x00004000
#define LR_SHARED           0x00008000

#define SM_CXSCREEN             0
#define SM_CYSCREEN             1

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

// hacks

// This is an ugly hack, fseek on windows using SEEK_CUR is undefined behaviour
// but the game relies heavily on it. What we do is emulate windows behaviour
// as the game expects here in linux
int fseek1(FILE*, int, int);

#endif


#endif
