#include "ZSutilities.h"
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <inttypes.h>

#ifdef _WIN32
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#endif

#if defined(__linux__) || defined(__APPLE__)
#include "linux_aux_wrapper.h"
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <regex.h>
#endif
#ifdef __linux__
#include <openssl/sha.h>
#endif

char *ExitErrorMessage = NULL;

const char *StatDescriptors[] = 
{
	"Worse than none", //less than zero
	"None",				// = 0
	"Bare Minimum",		// < 10
	"Awful",			// < 20
	"Very Bad",			// < 30
	"Bad",				// < 40
	"Below Average",	// < 50
	"Average",			// < 60
	"Above Average",	// < 70
	"Good",				// < 80
	"Very Good",		// < 90
	"Excellent",		// < 100
	"Perfect",			// = 100
	"Beyond Perfect"	// >100
};

const char *HealthDescriptors[] = 
{
	"Seriously Dead",		//less than zero
	"Dead",					// = 0
	"Almost Dead",			// < 10
	"Seriously Wounded",	// < 20
	"Wounded",				// < 30
	"Wounded",				// < 40
	"Injured",				// < 50
	"Bleeding",				// < 60
	"Bleeding",				// < 70
	"Bruised",				// < 80
	"Bruised",				// < 90
	"Scratched",			// < 100
	"Uninjured",			// = 100
	"Beyond Perfect"		// >100
};

const char *RestDescriptors[] = 
{
	"Unconscious", //less than zero
	"Collapsed",			// = 0
	"About to Collapse",	// < 10
	"Barely Standing",	// < 20
	"Very Bad",				// < 30
	"Bad",					// < 40
	"Heavily Winded",		// < 50
	"Winded",				// < 60
	"Winded",				// < 70
	"Breathing Hard",		// < 80
	"Barely Winded",		// < 90
	"Well Rested",			// < 100
	"Perfect",				// = 100
	"Beyond Perfect"		// >100
};

const char *SkillDescriptors[] = 
{
	"Worse than none",//less than zero
	"None",				// = 0
	"Minimal",			// < 10
	"Below Average",	// < 20
	"Average",			// < 30
	"Above Average",	// < 40
	"Good",				// < 50
	"Very Good",		// < 60
	"Exceptional",		// < 70
	"Master",			// < 80
	"GrandMaster",		// < 90
	"Perfect",			// < 100
	"SuperHuman",		// = 100
	"SuperHuman",		// >100
};

//attempts to open a file and exits if it fails

FILE *SafeFileOpen(const char *filename, const char *attributestring)
{
	PTD_ASSERT(filename);

	FILE* fp = fopen(filename, attributestring);

	if(fp == NULL)
	{
		char blarg[128];
	
		sprintf(blarg,"Failed to open file: %s.  Aborting program.",filename);
		SafeExit(blarg);
	}

	return fp;
}

//The game owns the screen while it is stuck, so alt-tabbing out to run a dump
//tool isn't possible - the only way out is to kill it, and then there is
//nothing left to look at.  Flip bumps this counter; a watchdog thread writes
//the dump itself when it stops moving.
static volatile LONG FrameTick = 0;

void PreludeFrameTick()
{
	InterlockedIncrement(&FrameTick);
}

#ifdef _WIN32
static DWORD WINAPI HangWatchdog(LPVOID)
{
	//Long enough that loading a save or an area can never trip it, short enough
	//to have caught it by the time anyone reaches for the task manager.
	static const int SECONDS_TO_CALL_IT_HUNG = 20;

	LONG LastSeen = FrameTick;
	int Still = 0;
	BOOL Dumped = FALSE;

	while(TRUE)
	{
		Sleep(1000);

		LONG Now = FrameTick;

		if(Now != LastSeen)
		{
			LastSeen = Now;
			Still = 0;
			continue;
		}

		Still++;

		if(Still < SECONDS_TO_CALL_IT_HUNG || Dumped)
			continue;

		//once per run: a stuck game would otherwise rewrite this every 20s
		Dumped = TRUE;

		HANDLE hFile = CreateFileA("hang.dmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			continue;

		//no exception to describe, so this is thread stacks and what they point
		//at - which is the whole question for a hang
		BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
			(MINIDUMP_TYPE)(MiniDumpWithHandleData | MiniDumpWithIndirectlyReferencedMemory), NULL, 0, 0);

		if(!ok)
		{
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, NULL, 0, 0);
		}

		CloseHandle(hFile);

		DEBUG_INFO(ok ? "no frame drawn for 20 seconds, wrote hang.dmp\n"
					  : "no frame drawn for 20 seconds, could not write hang.dmp\n");
	}

	return 0;
}
#endif

void StartHangWatchdog()
{
#ifdef _WIN32
	DWORD ThreadId;
	HANDLE hThread = CreateThread(NULL, 0, HangWatchdog, NULL, 0, &ThreadId);
	if(hThread)
		CloseHandle(hThread);
#endif
}

#ifdef _WIN32
void BacktraceToString(char* pBuffer, size_t bufferSize, PVOID* backTrace, size_t frameCount)
{
	for (size_t i = 0; i < frameCount; ++i)
	{
		char storageForSymbol[512];
		SYMBOL_INFO* pSymbol = (SYMBOL_INFO*)storageForSymbol;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = sizeof(storageForSymbol) - sizeof(SYMBOL_INFO) + 1;

		DWORD64 displacement;

		BOOL ok = SymFromAddr(GetCurrentProcess(), (DWORD64)backTrace[i], &displacement, pSymbol);
		size_t bufferLen = strlen(pBuffer);  // "Nice" n^2 :D
		size_t remainingBufferSize = bufferSize - bufferLen;
		char* pCurBuffer = pBuffer + bufferLen;
		if (ok)
		{
			snprintf(pCurBuffer, remainingBufferSize, "  %s+0x%x\n", pSymbol->Name, (unsigned int)displacement);
		}
		else
		{
			snprintf(pCurBuffer, remainingBufferSize, "  0x%016" PRIx64 "\n", (uint64_t)backTrace[i]);
		}
	}
}

void GetBacktrace(char* pBuffer, size_t bufferSize, size_t skipFrameCount = 0)
{
	if (bufferSize == 0)
		return;

	pBuffer[0] = '\0';

	ULONG framesToSkip = (ULONG)skipFrameCount;
	PVOID backTrace[64];
	USHORT capturedFrames = CaptureStackBackTrace(framesToSkip, 63 - framesToSkip, backTrace, NULL);

	BacktraceToString(pBuffer, bufferSize, backTrace, capturedFrames);
}


//A C++ throw reaches the filter as exception 0xE06D7363 with the compiler's
//ThrowInfo in ExceptionInformation[2].  Walking it turns the useless
//"Exception (3765269347)" into the name of the type that was thrown, which for
//a bad_alloc or a bad_array_new_length is most of the diagnosis.  The pointers
//are absolute on x86 and RVAs from ExceptionInformation[3] on x64; treating
//them as DWORDs plus a base of zero covers both.
static void GetCxxThrowTypeName(const EXCEPTION_RECORD *pRecord, char *pBuffer, size_t bufferSize)
{
	pBuffer[0] = '\0';

	if(pRecord->ExceptionCode != 0xE06D7363 || pRecord->NumberParameters < 3)
		return;

	uintptr_t Base = (pRecord->NumberParameters >= 4) ? (uintptr_t)pRecord->ExceptionInformation[3] : 0;

	__try
	{
		//ThrowInfo: attributes, unwind, forward compat, catchable type array
		const DWORD *pThrowInfo = (const DWORD *)pRecord->ExceptionInformation[2];
		if(!pThrowInfo || !pThrowInfo[3])
			return;

		const DWORD *pCatchableArray = (const DWORD *)(Base + pThrowInfo[3]);
		if(pCatchableArray[0] < 1 || !pCatchableArray[1])
			return;

		//CatchableType: properties, type descriptor, ...
		const DWORD *pCatchable = (const DWORD *)(Base + pCatchableArray[1]);
		if(!pCatchable[1])
			return;

		//TypeDescriptor: vftable, spare, then the mangled name
		const char *pName = (const char *)(Base + pCatchable[1]) + 2 * sizeof(void *);

		strncpy(pBuffer, pName, bufferSize - 1);
		pBuffer[bufferSize - 1] = '\0';
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		pBuffer[0] = '\0';
	}
}

LONG WINAPI PreludeUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
	static const size_t EXCEPTION_SHORT_INFO_SIZE = 256;
	char exceptionShortInfo[EXCEPTION_SHORT_INFO_SIZE];
	switch (pExceptionInfo->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
	{
		const char* access;
		switch (pExceptionInfo->ExceptionRecord->ExceptionInformation[0])
		{
		case 0:
			access = "read";
			break;
		case 1:
			access = "write";
			break;
		default:
			access = "unknown";
			break;
		}
		#if _WIN64
		snprintf(exceptionShortInfo, EXCEPTION_SHORT_INFO_SIZE, "ACCESS_VIOLATION %s@0x%016" PRIx64 " IP=0x%016" PRIx64,
			access, static_cast<uint64_t>(pExceptionInfo->ExceptionRecord->ExceptionInformation[1]), pExceptionInfo->ContextRecord->Rip);
		#else	
		snprintf(exceptionShortInfo, EXCEPTION_SHORT_INFO_SIZE, "ACCESS_VIOLATION %s@0x%08" PRIx32 " IP=0x%08" PRIx32,
			access, static_cast<uint32_t>(pExceptionInfo->ExceptionRecord->ExceptionInformation[1]), pExceptionInfo->ContextRecord->Eip);
		#endif
		break;
	}
	default:
	{
		char thrownType[128];
		GetCxxThrowTypeName(pExceptionInfo->ExceptionRecord, thrownType, sizeof(thrownType));
#if _WIN64
		snprintf(exceptionShortInfo, EXCEPTION_SHORT_INFO_SIZE, "Exception (%u) IP=0x%016" PRIx64 " %s", unsigned(pExceptionInfo->ExceptionRecord->ExceptionCode), (uint64_t)pExceptionInfo->ContextRecord->Rip, thrownType);
#else
		//Eip is 32 bits; printing it with a 64-bit conversion used to splice in
		//whatever followed it on the stack and report an impossible address
		snprintf(exceptionShortInfo, EXCEPTION_SHORT_INFO_SIZE, "Exception (%u) IP=0x%08" PRIx32 " %s", unsigned(pExceptionInfo->ExceptionRecord->ExceptionCode), (uint32_t)pExceptionInfo->ContextRecord->Eip, thrownType);
#endif
	}
	}


	char backtrace[1024];
	GetBacktrace(backtrace, sizeof(backtrace), 7);

	BOOL dumped = FALSE;

	HANDLE hFile = CreateFileA("core.dmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
		exceptionInfo.ThreadId = GetCurrentThreadId();
		exceptionInfo.ExceptionPointers = pExceptionInfo;
		exceptionInfo.ClientPointers = FALSE;
		dumped = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithIndirectlyReferencedMemory, &exceptionInfo, 0, 0);
		if (!dumped)
		{
			//that flag walks the heap for referenced memory, so it is the first
			//thing to fail when the crash was itself an allocation failure -
			//exactly the case where we most want a stack.  Leave a plain one.
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			SetEndOfFile(hFile);
			dumped = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exceptionInfo, 0, 0);
		}
		CloseHandle(hFile);
	}

	//an exhausted address space is its own explanation, and this is the one
	//moment we can still read it
	MEMORYSTATUSEX memory;
	memory.dwLength = sizeof(memory);
	char memoryInfo[64];
	if (GlobalMemoryStatusEx(&memory))
		snprintf(memoryInfo, sizeof(memoryInfo), "VA free: %llu MB\n", (unsigned long long)(memory.ullAvailVirtual >> 20));
	else
		memoryInfo[0] = '\0';

	char message[2048];
	snprintf(message, sizeof(message), "%s\n\n%s%s%s", exceptionShortInfo, dumped ? "A core was dumped\n" : "", memoryInfo, backtrace);

	MessageBoxA(NULL, message, "Exception", MB_OK);

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif

void SafeExit(const char *ErrorMessage)
{
	char exitErrorMessage[1024];
	sprintf(exitErrorMessage, "%s", ErrorMessage);

#ifdef _WIN32
	strcat_s(exitErrorMessage, sizeof(exitErrorMessage), "\n");
	char backtrace[512];
	GetBacktrace(backtrace, sizeof(backtrace));
	strcat_s(exitErrorMessage, sizeof(exitErrorMessage), backtrace);

	OutputDebugString(exitErrorMessage);
	
	MessageBoxA(NULL, exitErrorMessage, "Unexpected exit", MB_OK);
#endif

	DEBUG_INFO(exitErrorMessage);
	exit(1);
}

void PtdAssertFailed(const char* file, int line, const char* pred)
{
	char exitErrorMessage[1024];
	snprintf(exitErrorMessage, sizeof(exitErrorMessage), "Assertion failed: %s %s(%d)\n", pred, file, line);

#ifdef _WIN32
	char backtrace[512];
	GetBacktrace(backtrace, sizeof(backtrace), 1);
	strcat_s(exitErrorMessage, sizeof(exitErrorMessage), backtrace);

	OutputDebugString(exitErrorMessage);
#else
	fprintf(stderr, "%s", exitErrorMessage);
#endif

	DEBUG_INFO(exitErrorMessage);

#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		__debugbreak();
		return;
	}
	else
	{
		MessageBoxA(NULL, exitErrorMessage, "Assertion failed", MB_OK);
	}
#endif
	SafeExit(pred);
}

////////////////
///
//  gets a generic data type 
//
///////////////////

float GetFloat(FILE *fp)
{
	char blarg[16];

	int ccount = 0;

	char c = '\0';;
	 
	c = (char)fgetc(fp);

	while(!isdigit(c) && c != '-')
	{
		c = (char)fgetc(fp);
	}

	while(isdigit(c) || c == '.' || c == '-')
	{
		blarg[ccount] = c;
		c = (char)fgetc(fp);
		ccount++;
	}
	
	blarg[ccount] = '\0';

	return (float)atof(blarg);
}

int GetInt(FILE *fp)
{
	char blarg[16];

	int ccount = 0;

	char c = '\0';
	 
	c = (char)fgetc(fp);

	while(!isdigit(c) && c != '-')
	{
		c = (char)fgetc(fp);
	}

	while(isdigit(c) || c == '-')
	{
		blarg[ccount] = c;
		c = (char)fgetc(fp);
		ccount++;
	}
	
	blarg[ccount] = '\0';

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif

	return atoi(blarg);

}

DATA_T GetNumber(FILE *fp, DATA_FIELD_T *dest)
{
	char blarg[16];	//string to hold character representation of number

	int ccount = 0; //counter

	char c;			//read character

	c = (char)fgetc(fp);

	while(!isdigit(c) && c != '-')
	{
		c = (char)fgetc(fp);
	}

	//read in the character representation
	while (c == '-' || isdigit(c) || c == '.')
	{
		blarg[ccount] = c;
		c = (char)fgetc(fp);
		ccount++;
	}

	//terminate the number string
	blarg[ccount] = '\0';

	//convert it to a number
	char *pc;

	//check for a decimal point
	pc = strchr(blarg,'.');

	//if there is a decimal point convert to a float
	if(pc)
	{
		dest->fValue = (float)atof(blarg);
		return DATA_FLOAT;
	}
	else //otherwise convert to an integer
	{
		dest->Value = atoi(blarg);
		return DATA_INT;
	}

	
	return DATA_ERROR;

}

char *GetString(FILE *fp, DATA_FIELD_T *dest)
{
	fpos_t start;
	//get string gets called one character into the string, so seek back one

	//store the start position of the string
	fgetpos(fp,&start);

	int length = 0;

	char c = '\0';

	//count how long the string is
	while (c != DELIMIT_CHARACTER && c != '\n' && c != '\r')
	{
		length++;
		c = (char)fgetc(fp);
	}

	//return to the start of the string
	fsetpos(fp,&start);

	char *returnstring;

	//allocate space for the string, do so in 8 byte increments to avoid memory block issues
	returnstring = new char[length];

	//get the actual string
	fgets(returnstring,length,fp);

	c = (char)fgetc(fp);

	PTD_ASSERT((c == DELIMIT_CHARACTER) || (c == '\r') || (c == '\n'));

	//done
	dest->String = returnstring;
	return returnstring;
}

char *GetString(FILE *fp, char Delimitter, char * dest, int size) {
	fpos_t start;

	char *returnstring = dest;

	//store the start position of the string
	fgetpos(fp, &start);

	int length = 0;

	char c = '\0';

	//count how long the string is
	do {
		length++;
		c = (char)fgetc(fp);
#ifndef NDEBUG
		//validate character;	
		if (c == '[' || c == '(' || (c == '^' && Delimitter != '^')) {
			DEBUG_INFO("Bad character found in delimitting string");
			break;
		}
#endif
	} while (c != Delimitter && c != '\n' && c != '\r' && !feof(fp));

	//return to the start of the string
	fsetpos(fp, &start);

	PTD_ASSERT(length <= size);

	//allocate space for the string, do so in 8 byte increments to avoid memory block issues
	

	//get the actual string
	fgets(returnstring, length, fp);

	c = (char)fgetc(fp);

	if (c != Delimitter) {
		DEBUG_INFO("Problem with delimitted string\n");
		DEBUG_INFO(returnstring);
		DEBUG_INFO("\n");
		delete[] returnstring;
		exit(1);
	}
	//done
	return returnstring;
}

char *GetString(FILE *fp, char Delimitter)
{
	fpos_t start;

	//store the start position of the string
	fgetpos(fp,&start);

	int length = 0;

	char c = '\0';

	//count how long the string is
	do
	{
		length++;
		c = (char)fgetc(fp);
#ifndef NDEBUG
	//validate character;	
	if(c == '[' || c == '(' || (c == '^' && Delimitter != '^'))
	{
		DEBUG_INFO("Bad character found in delimitting string");
		break;
	}
#endif
	}while (c != Delimitter && c != '\n' && c != '\r' && !feof(fp));

	//return to the start of the string
	fsetpos(fp,&start);

	char *returnstring;

	//allocate space for the string, do so in 8 byte increments to avoid memory block issues
	returnstring = new char[length];

	//get the actual string
	fgets(returnstring,length,fp);

	c = (char)fgetc(fp);

	if(c != Delimitter)
	{
		DEBUG_INFO("Problem with delimitted string\n");
		DEBUG_INFO(returnstring);
		DEBUG_INFO("\n");
		delete[] returnstring;
		exit(1);
	}
	//done
	return returnstring;
}

char *GetString(FILE *fp, char *dest, char Delimitter)
{
	fpos_t start;

	//store the start position of the string
	fgetpos(fp,&start);

	int length = 0;

	char c = '\0';

	//count how long the string is
	do
	{
		length++;
		c = (char)fgetc(fp);
#ifndef NDEBUG
	//validate character;	
	if(c == '[' || c == '(' || (c == '^' && Delimitter != '^'))
	{
		DEBUG_INFO("Bad character found in delimitting string");
		break;
	}
#endif
	}while (c != Delimitter && c != '\n' && c != '\r' && !feof(fp));

	//return to the start of the string
	fsetpos(fp,&start);

	//get the actual string
	fgets(dest, length,fp);

	c = (char)fgetc(fp);

	if(c != Delimitter)
	{
		DEBUG_INFO("Problem with delimitted string\n");
		DEBUG_INFO(dest);
		DEBUG_INFO("\n");
		exit(1);
	}
	//done
	return dest;
}

char *GetString(FILE *fp, char *dest)
{
	int length = 0;
	char c = '\0';
	fpos_t start;

	while(c != '"')
	{
		if(c == EOF)
		{
			
		}
		c = fgetc(fp);
	}
	c = '\0';
	
	fgetpos(fp,&start);
		
	while (c != '"' && c != '\n' && c != '\r')
	{
		length++;
		c = fgetc(fp);
	}

	fsetpos(fp,&start);

	fgets(dest,length,fp);

	dest[length] = '\0';
	fgetc(fp);
	
	return dest;


}


char *GetString(FILE *fp)
{
	fpos_t start;
	//get string gets called one character into the string, so seek back one

	//store the start position of the string
	fgetpos(fp,&start);

	int length = 0;

	char c = '\0';

	//count how long the string is
	while (c != DELIMIT_CHARACTER && c != '\n' && c != '\r')
	{
		length++;
		c = (char)fgetc(fp);
	}

	//return to the start of the string
	fsetpos(fp,&start);

	char *returnstring;

	//allocate space for the string, do so in 8 byte increments to avoid memory block issues
	returnstring = new char[length];

	//get the actual string
	fgets(returnstring,length,fp);

	//done
	return returnstring;
}

D3DVECTOR *GetVector(FILE *fp, DATA_FIELD_T *dest)
{
	D3DVECTOR *NewVector;
	NewVector = new D3DVECTOR;
	NewVector->x = GetFloat(fp);
	NewVector->y = GetFloat(fp);
	NewVector->z = GetFloat(fp);
	dest->pVector = NewVector;
	char c;
	c = fgetc(fp);

	PTD_ASSERT(c == DELIMIT_CHARACTER);
	return NewVector;

}

DATA_T GetFileData(FILE *fp, DATA_FIELD_T *dest)
{
	char c;
	DATA_T ReturnValue;

	c = fgetc(fp);

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif

	if(c == '(')
	{
		dest->pVector = GetVector(fp, dest);
		return DATA_VECTOR;
	}
	
	if(isdigit(c) || c == '-')
	{
		ReturnValue = GetNumber(fp, dest);
		return ReturnValue;
	}

	if(isalpha(c))
	{
		dest->String = GetString(fp, dest);
		return DATA_STRING;
	}

	if(c == DELIMIT_CHARACTER)
	{
		fgetc(fp);
		dest->Value = 0;
		DEBUG_INFO("\n************\n\nDATA FIELD EMPTY\n\n******************\n\n");
		return DATA_NONE;
	}

	return DATA_ERROR;

}

int SaveInt(FILE *fp, int ToSave)
{
	fprintf(fp,"%i",ToSave);
	return TRUE;
}

int SaveFloat(FILE *fp, float ToSave)
{
	fprintf(fp,"%f",ToSave);
	return TRUE;
}

int SaveString(FILE *fp, const char *ToSave)
{
	fprintf(fp,"%s",ToSave);
	return TRUE;

}

int SaveVector(FILE *fp, D3DVECTOR *ToSave)
{
	fprintf(fp,"(%f,%f,%f)",(double)ToSave->x,(double)ToSave->y,(double)ToSave->z);
	return TRUE;
}

float GetDistance(D3DVECTOR *VA,D3DVECTOR *VB)
{
	float Length1;
	float Length2;

	Length1 = VA->x - VB->x;
	Length2 = VA->y - VB->y;
	
	return (float)sqrt((float)((Length1 * Length1) + (Length2 * Length2)));
}

int SeekTo(FILE *fp, const char *id)
{
	PTD_ASSERT(fp);

	char c = '\0';
	int n;
	BOOL found = FALSE;
	
	while (!found && !feof(fp))
	{
	
		while (c != id[0] && !feof(fp))
		{
			c = fgetc(fp);
		}
		
		n = 0;
		while (c == id[n] &&!feof(fp))
		{
			n++;
			c = fgetc(fp);
		}

		if(id[n] == '\0')
		{
			found = TRUE;
		}
	}

	if(!found)
	{
		DEBUG_INFO("Failed to seek to: ");
		DEBUG_INFO(id);
		DEBUG_INFO("\n");
		return FALSE;
	}

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif
	
	return TRUE;
}

int SeekToSkip(FILE *fp, const char *id)
{
	PTD_ASSERT(fp);

	char c = '\0';
	int n;
	BOOL found = FALSE;

	while (!found && !feof(fp))
	{
				
		while (c != id[0] && !feof(fp))
		{
			c = fgetc(fp);

			if(c == '[' && id[0] != '[')
			{
				while (c != ']' && !feof(fp))
				{
					c = fgetc(fp);
				}
			}
			else
			if(c == ';')
			{
				do
				{
					c = fgetc(fp);
				}while (c != ';' && !feof(fp));
			}

		}
		
		n = 0;
		while (c == id[n] &&!feof(fp))
		{
			n++;
			c = fgetc(fp);
		}

		if(id[n] == '\0')
		{
			found = TRUE;
		}
	}

	if(!found)
	{
		DEBUG_INFO("Failed to seek to: ");
		DEBUG_INFO(id);
		DEBUG_INFO("\n");
		return FALSE;
	}

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif

	return TRUE;
}

char *GetPureString(FILE *fp)
{
	fpos_t start;
	//get string gets called one character into the string, so seek back one

	int length = 0;

	char c = ' ';

	while(!isalpha(c))
	{
		c = (char)fgetc(fp);
	}

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif

	//store the start position of the string
	fgetpos(fp,&start);

	//count how long the string is
	while (isalpha((int)c) || isdigit((int)c))
	{
		length++;
		c = (char)fgetc(fp);
	}

	//return to the start of the string
	fsetpos(fp,&start);

	char *returnstring;

	//allocate space for the string, do so in 8 byte increments to avoid memory block issues
	returnstring = new char[length];

	//get the actual string
	fgets(returnstring,length,fp);

	//done
	return returnstring;
}


char *GetStringNoWhite(FILE *fp)
{
	fpos_t start;
	//get string gets called one character into the string, so seek back one

	int length = 0;

	char c = ' ';

	while(!isalpha(c))
	{
		c = (char)fgetc(fp);
	}

#if defined(__linux__) || defined(__APPLE__)
	fseek1(fp, -1, SEEK_CUR);
#elif _WIN32
	fseek(fp, -1, SEEK_CUR);
#endif

	//store the start position of the string
	fgetpos(fp,&start);

	//count how long the string is
	while (!isspace((int)c))
	{
		length++;
		c = (char)fgetc(fp);
	}

	//return to the start of the string
	fsetpos(fp,&start);

	char *returnstring;

	//allocate space for the string
	returnstring = new char[length];

	//get the actual string
	fgets(returnstring, length, fp);

	//done
	return returnstring;
}

DIRECTION_T	FindFacing(D3DVECTOR *pvFrom, D3DVECTOR *pvTo)
{
	float angle;

	D3DVECTOR v1;
	D3DVECTOR v2;

	v1.x = 0.0f;
	v1.y = 1.0f;
	v1.z = 0.0f;
	
	v2.z = 0;

	v2.x = pvTo->x - pvFrom->x;
	v2.y = -(pvTo->y - pvFrom->y);

	angle = RadToDeg(GetAngle(&v1,&v2));

	int Angle = (int)angle;

	if(Angle < 23)
	{
		return NORTH;
	}
	else
	if(Angle < 68)
	{
		return NORTHEAST;
	}
	else
	if(Angle < 113)
	{
		return EAST;
	}
	else
	if(Angle < 158)
	{ 
		return SOUTHEAST;
	}
	else
	if(Angle < 203)
	{
		return SOUTH;
	}
	else
	if(Angle < 248)
	{
		return SOUTHWEST;
	}
	else
	if(Angle < 293)
	{
		return WEST;
	}
	else
	if(Angle < 337)
	{
		return NORTHWEST;
	}
	else
	{
		return NORTH;
	}
	
	return DIR_NONE;
}

DIRECTION_T	FindFacing(float Angle)
{
	int NewAngle;
	NewAngle = (int)RadToDeg(Angle);

	if(NewAngle < 23)
	{
		return NORTH;
	}
	else
	if(NewAngle < 68)
	{
		return NORTHEAST;
	}
	else
	if(NewAngle < 113)
	{
		return EAST;
	}
	else
	if(NewAngle < 158)
	{ 
		return SOUTHEAST;
	}
	else
	if(NewAngle < 203)
	{
		return SOUTH;
	}
	else
	if(NewAngle < 248)
	{
		return SOUTHWEST;
	}
	else
	if(NewAngle < 293)
	{
		return WEST;
	}
	else
	if(NewAngle < 337)
	{
		return NORTHWEST;
	}
	else
	{
		return NORTH;
	}

}


int ConvertToPercent(float f)
{

	return (int)(f * 100.0f);
}

float ConvertFromPercent(int n)
{
	return (float)n/100.0f;
}


float GetAngle(D3DVECTOR *v1, D3DVECTOR *v2)
{
//	float CosAngle = DotProduct(*v1,*v2)/((Magnitude(*v1)*Magnitude(*v2)));
//	float Angle = (float)acos(CosAngle);

	float DegAngle, Angle;

	D3DVECTOR vNorth(0.0f,1.0f,0.0f);
	float AngleA;
	float AngleB;

	AngleA = (float)acos(DotProduct(vNorth,*v1)/Magnitude(*v1));
	AngleB = (float)acos(DotProduct(vNorth,*v2)/Magnitude(*v2));

	if(v1->x < 0.0f) AngleA = PI_MUL_2 - AngleA;
	if(v2->x < 0.0f) AngleB = PI_MUL_2 - AngleB;
	 
	Angle = AngleB - AngleA;

	if(Angle < 0)
	{
		Angle = PI_MUL_2 + Angle;
	}
	
	DegAngle = (float)RadToDeg(Angle);

	return Angle;


//	return Angle;
}

//distance function
//point to point
float GetDist(float x1,float y1, float x2, float y2)
{
	return (float)sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

//point to line
//point to test, then start and end points of line
float GetDist(float x, float y, float x1, float y1, float x2, float y2)
{
	float dist;
	float angle;
	dist = GetDist(x,y,x1,y1);
	D3DVECTOR a;
	D3DVECTOR b;
	a.x = x2 - x1;
	a.y = y2 - y1;
	b.x = x - x1;
	b.y = y - y1;
	
	angle = GetAngle(&a,&b);

	float top, bottom, left, right;
	if(y1 > y2)
	{
		top = y2;
		bottom = y1;
	}
	else
	{
		top = y1;
		bottom = y2;
	}
	if(x1 > x2)
	{
		right = x1;
		left = x2;
	}
	else
	{
		right = x2;
		left = x1;
	}

	float nx = x1 + (dist *(float)sin(angle));
	float ny = y1 + (dist *(float)cos(angle));

	if(nx < left || nx > right || ny < top || ny > bottom)
	{
		return 10000.0f;
	}
	
	return dist*(float)sin(angle);

}


float AngleDifCW(const float Angle1, const float Angle2)
{
	if(Angle2 > Angle1)
	{
		return Angle2 - Angle1;
	}
	else
	{
		return Angle2 + (PI_MUL_2 - Angle1);
	}
}


float AngleDifCCW(const float Angle1, const float Angle2)
{
	if(Angle1 > Angle2)
	{
		return Angle1 - Angle2;
	}
	else
	{
		return Angle1 + (PI_MUL_2 - Angle2);
	}
}


//get next printable character from a file, ignoring comments
char GetChar(FILE *fp)
{
	char c;

	do
	{
		c = fgetc(fp);
		if(c == ';')
		{
			do
			{
				c = fgetc(fp);
			}
			while(c != ';');
			c = fgetc(fp);
		}
	}while(!feof(fp) && isspace((int)c));
	

	return c;
}


char *GetRangeDescriptor(int Num, int Min, int Max, RANGE_T RangeType)
{
	int RangeIndex;
	int RangeLength = Max - Min;
	int RangeNum = Num - Min;

	PTD_ASSERT(RangeLength);

	//calculate where in the range the number is
	if(RangeNum < 0)
	{
		RangeIndex = RANGE_DESCRIPTOR_START;
	}
	else
	if(RangeNum == 0)
	{
		RangeIndex = RANGE_DESCRIPTOR_START + 1;
	}
	else
	if(RangeNum == RangeLength)
	{
		RangeIndex = RANGE_DESCRIPTOR_END - 1;
	}
	else
	if(RangeNum > RangeLength)
	{
		RangeIndex = RANGE_DESCRIPTOR_END;
	}
	else
	{
		RangeIndex = (int)(10.0f *((float)RangeNum/(float)RangeLength)) + 2;
	}

	switch(RangeType)
	{
		case RANGE_STAT:
			return (char *)StatDescriptors[RangeIndex];
		case RANGE_SKILL:
			return (char *)SkillDescriptors[RangeIndex];
		case RANGE_HEALTH:
			return (char *)HealthDescriptors[RangeIndex];
		case RANGE_REST:
			return (char *)RestDescriptors[RangeIndex];
		default:
			break;
	}

	return NULL;
}

void ConvertToCapitals(char *String)
{
	int n;
	n = 0;

	while(String[n] != '\0')
	{
		if(isalpha((int)String[n]))
			String[n] = (char)toupper((int)String[n]);
		n++;
	}
}

void ConvertToLowerCase(char *String)
{
	int n;
	n = 0;

	while(String[n] != '\0')
	{
		if(isalpha((int)String[n]))
			String[n] = (char)tolower((int)String[n]);
		n++;
	}
}

float PointToLine(D3DVECTOR *pPoint, D3DVECTOR *pStart, D3DVECTOR *pEnd)
{
	D3DVECTOR LineRay, PointRay;
	LineRay = *pEnd - *pStart;

//	LineRay = Normalize(LineRay);

	PointRay = *pPoint - *pStart;
//	float Distance;

//	Distance = DotProduct(PointRay, LineRay);

//	LineRay = LineRay * Distance;

//	LineRay += *pStart;

	float Angle;

	Angle = (float)acos(DotProduct(Normalize(PointRay),Normalize(LineRay)));

	float Hypotenuse = Magnitude(PointRay);

	return (float)sin(Angle)*Hypotenuse;

//	return GetDistance(&LineRay, pPoint);
}

int PointToLineIntersect(D3DVECTOR *vOut, D3DVECTOR *pPoint, D3DVECTOR *pStart, D3DVECTOR *pEnd)
{
	D3DVECTOR LineRay, PointRay;
	LineRay = *pEnd - *pStart;

	LineRay = Normalize(LineRay);

	PointRay = *pPoint - *pStart;

	float Distance;

	Distance = DotProduct(PointRay, LineRay);

	LineRay = LineRay * Distance;

	LineRay += *pStart;

	*vOut = LineRay; 

	return TRUE;

}

void ScaleRect(RECT *rToScale, RECT *Scalar, RECT *Base)
{
	float xfactor;

	xfactor = (float)(Base->right - Base->left) / (float)(Scalar->right - Scalar->left);

	float yfactor;

	yfactor = (float)(Base->bottom - Base->top) / (float)(Scalar->bottom - Scalar->top);
	

	rToScale->left = (int)((float)rToScale->left * xfactor);
	rToScale->right = (int)((float)rToScale->right * xfactor);
	rToScale->top = (int)((float)rToScale->top * yfactor);
	rToScale->bottom = (int)((float)rToScale->bottom * yfactor);
}

void LoadRect(RECT *rLoad, FILE *fp)
{
	rLoad->left = GetInt(fp);
	rLoad->top = GetInt(fp);
	rLoad->right = rLoad->left + GetInt(fp);
	rLoad->bottom = rLoad->top + GetInt(fp);
}

char *GetHelp(const char *HelpID)
{
	char *retstring = NULL;
	char UpCase[64];
	char Label[66];

	snprintf(Label, sizeof(Label), "[%s]", HelpID);

	FILE *fp;
	fp = SafeFileOpen("help.txt","rt");
	if(SeekTo(fp,Label))
	{
		SeekTo(fp,"\"");
		retstring = GetString(fp,'\"');
	}
	else
	{
		fseek(fp,0,0);
		strcpy(UpCase, HelpID);
		ConvertToCapitals(UpCase);
		snprintf(Label, sizeof(Label), "[%s]", UpCase);
		if(SeekTo(fp,Label))
		{
			SeekTo(fp,"\"");
			retstring = GetString(fp,'\"');
		}
	}

	fclose(fp);
	if(!retstring)
	{
		retstring = new char[128];
		sprintf(retstring,"help topic not available: %s\n", HelpID);
	}
	return retstring;
}

float ManhattanDistance(D3DVECTOR *VA, D3DVECTOR *VB)
{

	float XDif;
	float YDif;
	float ZDif;

	if(VA->x > VB->x)
	{
		XDif = VA->x - VB->x;
	}
	else
	{
		XDif = VB->x - VA->x;
	}

	if(VA->y > VB->y)
	{
		YDif = VA->y - VB->y;
	}
	else
	{
		YDif = VB->y - VA->y;
	}

	if(VA->z > VB->z)
	{
		ZDif = VA->z - VB->z;
	}
	else
	{
		ZDif = VB->z - VA->z;
	}

	return XDif + YDif + ZDif;
}

BOOL Triangle2DIntersect(D3DVECTOR *vPoint, D3DVERTEX *vxA, D3DVERTEX *vxB, D3DVERTEX *vxC)
{
	D3DVECTOR vTest;
	vTest = *vPoint;
	vTest.z = 0.0f;

	D3DVECTOR vA;
	vA.x = vxA->x - vTest.x;
	vA.y = vxA->y - vTest.y;
	vA.z = 0.0f;

	vA = Normalize(vA);

	D3DVECTOR vB;
	vB.x = vxB->x - vTest.x;
	vB.y = vxB->y - vTest.y;
	vB.z = 0.0f;

	vB= Normalize(vB);
	
	D3DVECTOR vC;
	vC.x = vxC->x - vTest.x;
	vC.y = vxC->y - vTest.y;
	vC.z = 0.0f;

	vC = Normalize(vC);

	float Angle = 0.0f;
	float Angle1 = 0.0f;
	float Angle2 = 0.0f;
	float Angle3 = 0.0f;

	//had to reverse my angle calculations from what I thought they should be...a-b-c to c-b-a???
	Angle1 = (float)acos(DotProduct(vA,vB));
	Angle2 = (float)acos(DotProduct(vB,vC));
	Angle3 = (float)acos(DotProduct(vC,vA));

	Angle = Angle1 + Angle2 + Angle3;

/*	if(Angle1 < 0.0f || Angle2 < 0.0f || Angle3 < 0.0f)
	{
		return FALSE;
	}
*/
	if(Angle > (PI_MUL_2 + 0.03f) || Angle < (PI_MUL_2 - 0.03f))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL Triangle2DIntersect(D3DVECTOR *vPoint, D3DVERTEX *VertArray, unsigned short *Indexes)
{
	return Triangle2DIntersect(vPoint, &VertArray[Indexes[0]], &VertArray[Indexes[1]], &VertArray[Indexes[2]]);
}

BOOL Triangle3DIntersect(D3DVECTOR *vRayStart, D3DVECTOR *vRayEnd, D3DVERTEX *vxA, D3DVERTEX *vxB, D3DVERTEX *vxC)
{
	D3DVECTOR vSideA;
	D3DVECTOR vSideB;
	D3DVECTOR vL1;
	D3DVECTOR vL2;
	D3DVECTOR vL3;
	D3DVECTOR vNormA;

	D3DVECTOR vA;
	vA.x = vxA->x;
	vA.y = vxA->y;
	vA.z = vxA->z;

	D3DVECTOR vB;
	vB.x = vxB->x;
	vB.y = vxB->y;
	vB.z = vxB->z;
	
	D3DVECTOR vC;
	vC.x = vxC->x;
	vC.y = vxC->y;
	vC.z = vxC->z;

	D3DVECTOR vIntersect;

	D3DVECTOR vRay;
	vRay = *vRayEnd - *vRayStart;

	float DistToPlane;
	float ProjectedLength;
	float Ratio;

	float AngleTotal;

	vSideA = vA - vB;

	vSideB = vC - vB;

	vNormA = Normalize(CrossProduct(vSideA,vSideB));
	
	//vNorm now contains the normal to the upper left triangle of this tile
	vL1 = vB - *vRayStart;

	DistToPlane = DotProduct(vL1,vNormA);
	
	ProjectedLength = DotProduct((*vRayEnd - *vRayStart),vNormA);
	Ratio = DistToPlane / ProjectedLength;

	vIntersect = *vRayStart + (vRay * Ratio);
	
	AngleTotal = 0.0f;
	//now check to see if the interect is in the polygon;
	vL1 = vA - vIntersect;
	vL1 = Normalize(vL1);

	vL2 = vB - vIntersect;
	vL2 = Normalize(vL2);

	vL3 = vC - vIntersect;
	vL3 = Normalize(vL3);

	AngleTotal += (float)acos(DotProduct(vL1,vL2));
	AngleTotal += (float)acos(DotProduct(vL2,vL3));
	AngleTotal += (float)acos(DotProduct(vL3,vL1));

	if(AngleTotal > 6.27f && AngleTotal < 6.29f)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL Triangle3DIntersect(D3DVECTOR *vRayStart, D3DVECTOR *vRayEnd, D3DVERTEX *VertArray, unsigned short *Indexes)
{
	return Triangle3DIntersect(vRayStart, vRayEnd, &VertArray[Indexes[0]], &VertArray[Indexes[1]], &VertArray[Indexes[2]]);
}

BOOL Quad3DIntersect(D3DVECTOR *vRayStart, D3DVECTOR *vRayEnd, D3DVERTEX *VertArray, unsigned short *Indexes)
{
	return Quad3DIntersect(vRayStart, vRayEnd, &VertArray[Indexes[0]], &VertArray[Indexes[1]], &VertArray[Indexes[2]], &VertArray[Indexes[4]]);
}

BOOL Quad3DIntersect(D3DVECTOR *vRayStart, D3DVECTOR *vRayEnd, D3DVERTEX *vxA, D3DVERTEX *vxB, D3DVERTEX *vxC, D3DVERTEX *vxD)
{
	
	return FALSE;
}



int GenericFindFiles(char * pattern, std::vector<findFilesData> * data) {
#if defined(__linux__) || defined(__APPLE__)
	DIR *d;
	regex_t    re;

	// This loop was copied from ClearHashedEvents(), where deleting the matches
	// is the point. Here the matches are the player's savegames.
	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB)) {
		return -1;
	}

	struct dirent *dire;
	d = opendir(".");
	if (!d) {
		regfree(&re);
		return -1;
	}

	while ((dire = readdir(d)) != NULL) {
		if (!regexec(&re, dire->d_name, 0, 0, 0)) {
			findFilesData f;
			snprintf(f.name, sizeof(f.name), "%s", dire->d_name);
			(*data).push_back(f);
		}
	}

	closedir(d);
	regfree(&re);

	return (*data).size();
#else
	HANDLE h;
	WIN32_FIND_DATA fd;

	h = FindFirstFile(pattern, &fd);

	if (h == INVALID_HANDLE_VALUE) {
		return -1;
	}

	do {
		findFilesData f;
		strcpy(f.name, fd.cFileName);
		(*data).push_back(f);
	} while (FindNextFile(h, &fd));

	FindClose(h);

	return (*data).size();
#endif
}
