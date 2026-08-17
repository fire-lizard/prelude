/* Prelude minidump reader - what crashed, and where.
 *
 * The game installs an unhandled-exception filter (ZSutilities.cpp) that writes
 * core.dmp next to the exe. This reads that dump: the exception, the faulting
 * instruction with file and line, the thrown C++ type when there is one, and
 * the frames it can recover. Symbols come from the .pdb next to the exe, so
 * point it at the build output rather than the copy in the game directory.
 *
 * Build 32-bit, to match the game:
 *     vcvars32 && cl /nologo dumpread.c /link /out:dumpread.exe
 *
 * Run:
 *     dumpread.exe core.dmp out\build\x86-Release\Prelude.exe
 *     dumpread.exe core.dmp Prelude.exe 2A793EF0 8   dump 8 dwords at an address
 *
 * The second form walks structures the crash was holding: values that land
 * inside the exe get symbolized, which is how you read a ScriptBlock or a
 * vtable out of the wreckage.
 *
 * Two notes on the recovered frames. The ebp chain stops early wherever the
 * optimizer dropped frame pointers, so it is short but trustworthy. The scan
 * below it reads every stack word and keeps the ones a call instruction
 * actually ends at - it re-reads the instruction bytes from the exe on disk,
 * since dumps carry stack and heap but not the code pages. Stale frames from
 * earlier calls still survive that test, so read it as candidates, newest
 * first, not as a call stack.
 */
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

#pragma comment(lib, "dbghelp.lib")

static HANDLE hProc;
static DWORD64 modBase, modSize;

static MINIDUMP_MEMORY_LIST *gMem;      /* memory captured in the dump */
static void *gDumpBase;

static BYTE *gImage;                    /* the exe itself, mapped from disk */
static IMAGE_SECTION_HEADER *gSect;
static int gNumSect;

/* target memory, served out of the dump */
static BOOL CALLBACK ReadMem(HANDLE proc, DWORD64 addr, PVOID buf, DWORD bytes, LPDWORD read)
{
	(void)proc;
	for (ULONG32 i = 0; gMem && i < gMem->NumberOfMemoryRanges; i++) {
		MINIDUMP_MEMORY_DESCRIPTOR *d = &gMem->MemoryRanges[i];
		DWORD64 start = d->StartOfMemoryRange;
		if (addr >= start && addr + bytes <= start + d->Memory.DataSize) {
			memcpy(buf, (char *)gDumpBase + d->Memory.Rva + (addr - start), bytes);
			if (read) *read = bytes;
			return TRUE;
		}
	}
	if (read) *read = 0;
	return FALSE;
}

/* code and read-only data, served out of the exe: dumps don't carry them */
static BOOL ReadImage(DWORD addr, void *buf, int bytes)
{
	if (!gImage || addr < modBase) return FALSE;

	DWORD rva = (DWORD)(addr - modBase);

	for (int i = 0; i < gNumSect; i++) {
		DWORD va = gSect[i].VirtualAddress;
		DWORD sz = gSect[i].SizeOfRawData;
		if (rva >= va && rva + bytes <= va + sz) {
			memcpy(buf, gImage + gSect[i].PointerToRawData + (rva - va), bytes);
			return TRUE;
		}
	}
	return FALSE;
}

/* does a call instruction end exactly here?  filters stale stack junk */
static BOOL IsReturnAddress(DWORD ret)
{
	BYTE b[8];

	if (ReadImage(ret - 5, b, 5) && b[0] == 0xE8) {                                  /* call rel32   */
		DWORD target = ret + *(int *)(b + 1);
		if (target >= modBase && target < modBase + modSize) return TRUE;
	}
	if (ReadImage(ret - 6, b, 6) && b[0] == 0xFF && (b[1] & 0x38) == 0x10) return TRUE;  /* call [mem]   */
	if (ReadImage(ret - 3, b, 3) && b[0] == 0xFF && (b[1] & 0x38) == 0x10) return TRUE;  /* call [reg+d] */
	if (ReadImage(ret - 2, b, 2) && b[0] == 0xFF && (b[1] & 0xF8) == 0xD0) return TRUE;  /* call reg     */
	return FALSE;
}

static void Symbolize(DWORD addr, const char *prefix)
{
	char buf[sizeof(SYMBOL_INFO) + 512];
	SYMBOL_INFO *sym = (SYMBOL_INFO *)buf;
	IMAGEHLP_LINE64 line;
	DWORD lineDisp = 0;
	DWORD64 symDisp = 0;

	sym->SizeOfStruct = sizeof(SYMBOL_INFO);
	sym->MaxNameLen = 500;

	if (!SymFromAddr(hProc, addr, &symDisp, sym)) {
		printf("%s0x%08lX  <no symbol>\n", prefix, addr);
		return;
	}

	line.SizeOfStruct = sizeof(line);
	if (SymGetLineFromAddr64(hProc, addr, &lineDisp, &line))
		printf("%s0x%08lX  %s+0x%llX   %s:%lu\n", prefix, addr, sym->Name, symDisp, line.FileName, line.LineNumber);
	else
		printf("%s0x%08lX  %s+0x%llX\n", prefix, addr, sym->Name, symDisp);
}

/* an MSVC throw carries the type in its ThrowInfo, which the linker also names */
static void DescribeThrow(const MINIDUMP_EXCEPTION *rec)
{
	DWORD obj = (DWORD)rec->ExceptionInformation[1];
	DWORD throwInfo = (DWORD)rec->ExceptionInformation[2];
	DWORD cta = 0, count = 0, ct = 0, typeDesc = 0, what = 0, got = 0;
	char name[256] = {0}, msg[256] = {0};

	printf("\nC++ exception:\n");

	if (ReadMem(NULL, throwInfo + 12, &cta, 4, &got) && cta &&
	    ReadMem(NULL, cta, &count, 4, &got) && count &&
	    ReadMem(NULL, cta + 4, &ct, 4, &got) && ct &&
	    ReadMem(NULL, ct + 4, &typeDesc, 4, &got) && typeDesc) {
		for (int i = 0; i < 250; i++) {
			char c = 0;
			if (!ReadMem(NULL, typeDesc + 8 + i, &c, 1, &got) || !c) break;
			name[i] = c;
		}
		printf("  type: %s   (%lu catchable types)\n", name, count);
	}
	else {
		/* static data isn't in the dump; the throw info has a symbol though */
		Symbolize(throwInfo, "  throw info: ");
	}

	/* std::exception keeps its message behind the vftable pointer */
	if (ReadMem(NULL, obj + 4, &what, 4, &got) && what) {
		for (int i = 0; i < 250; i++) {
			char c = 0;
			if (!ReadMem(NULL, what + i, &c, 1, &got) || !c) break;
			msg[i] = c;
		}
		if (msg[0]) printf("  what(): %s\n", msg);
	}
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc < 3) {
		printf("usage: dumpread <core.dmp> <Prelude.exe> [hex address] [dwords]\n");
		return 1;
	}

	const char *dumpPath = argv[1];
	const char *exePath = argv[2];

	HANDLE hFile = CreateFileA(dumpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("can't open %s\n", dumpPath);
		return 1;
	}

	void *base = MapViewOfFile(CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL), FILE_MAP_READ, 0, 0, 0);
	if (!base) {
		printf("can't map %s\n", dumpPath);
		return 1;
	}

	MINIDUMP_DIRECTORY *dir;
	void *stream;
	ULONG size;

	/* A crash dump has an exception stream. A dump taken of a *hung* process
	   (Task Manager, "Create dump file") has none: there is no faulting thread,
	   so every thread gets walked instead - the one sitting in a loop is the
	   answer. */
	MINIDUMP_EXCEPTION_STREAM *ex = NULL;
	if (MiniDumpReadDumpStream(base, ExceptionStream, &dir, &stream, &size))
		ex = (MINIDUMP_EXCEPTION_STREAM *)stream;

	CONTEXT *ctx = ex ? (CONTEXT *)((char *)base + ex->ThreadContext.Rva) : NULL;

	if (MiniDumpReadDumpStream(base, MemoryListStream, &dir, &stream, &size)) {
		gMem = (MINIDUMP_MEMORY_LIST *)stream;
		gDumpBase = base;
	}

	if (!ex)
		printf("no exception stream: reading this as a hang dump\n");

	if (ex)
	printf("exception 0x%08lX  at 0x%08llX\n",
		ex->ExceptionRecord.ExceptionCode, ex->ExceptionRecord.ExceptionAddress);

	if (ex && ex->ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
	    ex->ExceptionRecord.NumberParameters >= 2) {
		static const char *how[] = { "read", "wrote", "?", "?", "?", "?", "?", "?", "executed" };
		printf("  %s address 0x%08llX\n",
			how[ex->ExceptionRecord.ExceptionInformation[0] & 7],
			ex->ExceptionRecord.ExceptionInformation[1]);
	}

	if (ctx)
		printf("  EIP=0x%08lX ESP=0x%08lX EBP=0x%08lX EAX=0x%08lX EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX ESI=0x%08lX EDI=0x%08lX\n",
			ctx->Eip, ctx->Esp, ctx->Ebp, ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi);

	for (ULONG32 p = 0; ex && p < ex->ExceptionRecord.NumberParameters && p < 15; p++)
		printf("  param[%lu] = 0x%08llX\n", p, ex->ExceptionRecord.ExceptionInformation[p]);

	/* the exe is loaded wherever ASLR put it: everything else is relative to that */
	MINIDUMP_MODULE_LIST *mods = NULL;
	if (MiniDumpReadDumpStream(base, ModuleListStream, &dir, &stream, &size))
		mods = (MINIDUMP_MODULE_LIST *)stream;

	for (ULONG32 i = 0; mods && i < mods->NumberOfModules; i++) {
		MINIDUMP_MODULE *m = &mods->Modules[i];
		MINIDUMP_STRING *ms = (MINIDUMP_STRING *)((char *)base + m->ModuleNameRva);
		if (wcsstr(ms->Buffer, L"Prelude.exe")) {
			modBase = m->BaseOfImage;
			modSize = m->SizeOfImage;
			if (ex)
				printf("  Prelude.exe base=0x%08llX size=0x%llX  (fault RVA 0x%llX)\n",
					modBase, modSize, ex->ExceptionRecord.ExceptionAddress - modBase);
			else
				printf("  Prelude.exe base=0x%08llX size=0x%llX\n", modBase, modSize);
		}
	}

	HANDLE hExe = CreateFileA(exePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hExe != INVALID_HANDLE_VALUE) {
		gImage = (BYTE *)MapViewOfFile(CreateFileMappingA(hExe, NULL, PAGE_READONLY, 0, 0, NULL), FILE_MAP_READ, 0, 0, 0);
		if (gImage) {
			IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)gImage;
			IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(gImage + dos->e_lfanew);
			gNumSect = nt->FileHeader.NumberOfSections;
			gSect = IMAGE_FIRST_SECTION(nt);
		}
	}

	hProc = GetCurrentProcess();
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
	SymInitialize(hProc, NULL, FALSE);
	if (!SymLoadModuleEx(hProc, NULL, exePath, NULL, modBase, (DWORD)modSize, NULL, 0))
		printf("  (no symbols: %lu - is the .pdb next to the exe?)\n", GetLastError());

	if (ctx) {
		printf("\nfaulting instruction:\n");
		Symbolize(ctx->Eip, "  ");
	}

	if (ex && ex->ExceptionRecord.ExceptionCode == 0xE06D7363 && ex->ExceptionRecord.NumberParameters >= 3)
		DescribeThrow(&ex->ExceptionRecord);

	/* address given: dump memory there instead of the stack */
	if (argc > 3) {
		DWORD addr = (DWORD)strtoul(argv[3], NULL, 16);
		int count = argc > 4 ? atoi(argv[4]) : 16;

		printf("\nmemory at 0x%08lX:\n", addr);
		for (int i = 0; i < count; i++) {
			DWORD v = 0, got = 0;
			if (!ReadMem(NULL, addr + i * 4, &v, 4, &got) || got != 4) {
				printf("  +%02d 0x%08lX  <not in dump>\n", i * 4, addr + i * 4);
				continue;
			}
			char txt[5] = {0};
			memcpy(txt, &v, 4);
			for (int c = 0; c < 4; c++) if (txt[c] < 32 || txt[c] > 126) txt[c] = '.';
			printf("  +%02d 0x%08lX = 0x%08lX  %10ld  '%s'\n", i * 4, addr + i * 4, v, (long)v, txt);
			if (v >= modBase && v < modBase + modSize)
				Symbolize(v, "        -> ");
		}
		return 0;
	}

	MINIDUMP_THREAD_LIST *threads = NULL;
	if (MiniDumpReadDumpStream(base, ThreadListStream, &dir, &stream, &size))
		threads = (MINIDUMP_THREAD_LIST *)stream;

	/* crash dump: only the faulting thread is interesting.
	   hang dump: no faulting thread, so walk them all and read the one that is
	   sitting in our code. */
	for (ULONG32 i = 0; threads && i < threads->NumberOfThreads; i++) {
		MINIDUMP_THREAD *t = &threads->Threads[i];

		if (ex && t->ThreadId != ex->ThreadId) continue;

		CONTEXT *tctx = (CONTEXT *)((char *)base + t->ThreadContext.Rva);
		if (ctx) tctx = ctx;

		printf("\n--- thread %lu  EIP=0x%08lX ESP=0x%08lX EBP=0x%08lX ---\n",
			t->ThreadId, tctx->Eip, tctx->Esp, tctx->Ebp);

		printf("stopped in:\n");
		Symbolize(tctx->Eip, "  ");

		printf("call stack (ebp chain, stops where frame pointers were optimized away):\n");
		{
			DWORD ebp = tctx->Ebp, ret = 0, next = 0;
			int depth = 0;

			while (depth++ < 32 && ebp) {
				DWORD got = 0;
				if (!ReadMem(NULL, ebp, &next, 4, &got) || got != 4) break;
				if (!ReadMem(NULL, ebp + 4, &ret, 4, &got) || got != 4) break;
				if (ret < modBase || ret >= modBase + modSize) break;
				Symbolize(ret, "  ");
				if (next <= ebp) break;
				ebp = next;
			}
		}

		DWORD64 stackStart = t->Stack.StartOfMemoryRange;
		DWORD *mem = (DWORD *)((char *)base + t->Stack.Memory.Rva);
		ULONG32 words = t->Stack.Memory.DataSize / 4;
		ULONG32 skip = 0;

		if (tctx->Esp > stackStart && tctx->Esp < stackStart + t->Stack.Memory.DataSize)
			skip = (ULONG32)((tctx->Esp - stackStart) / 4);

		printf("candidate callers (a call instruction ends at each; may include stale frames):\n");
		for (ULONG32 w = skip; w < words; w++) {
			DWORD v = mem[w];
			if (v >= modBase && v < modBase + modSize && IsReturnAddress(v))
				Symbolize(v, "  ");
		}
	}

	return 0;
}
