#ifndef SCRIPT_H
#define SCRIPT_H

#include "ZSwindow.h"
#include "Flags.h"
#include "creatures.h"
#include <stdio.h>

#define MAX_ARGS 128

typedef enum
{
	ARG_NONE,
	ARG_FUNC_ID,
	ARG_LABEL,
	ARG_CHARACTER_LABEL,
	ARG_STRING,
	ARG_BLOCK,
	ARG_NUMBER,
	ARG_TERMINATOR,
	ARG_THING,
	ARG_ITEM,
	ARG_FLAG,
	ARG_PARTY,
	ARG_CREATURE,
	ARG_LEADER,
	ARG_PARTYONE,
	ARG_PARTYTWO,
	ARG_PARTYTHREE,
	ARG_PARTYFOUR,
	ARG_PARTYFIVE,
	ARG_PARTYSIX,
} ARGUMENT_T;

class ScriptArg
{
private:
	void *Value;
	ARGUMENT_T Type;

public:
	void *GetValue()
	{
		if(Type > ARG_CREATURE)
		{
			return GetCreature();
		}
		else
		{
			return Value;
		}
		return NULL;
	};

	int GetIntValue()
	{
		void* pValue = GetValue();
		uintptr_t castValue = (uintptr_t)pValue;
		int result = (int)castValue;
		// A variant holds either a glorified int or a real object pointer, and
		// callers (iff, cond) use this as a truth test. Truncating to 32 bits is
		// what the 32-bit builds always did; the one case that must not happen
		// is a live pointer whose low word is zero reading back as false.
		if (((uintptr_t)result != castValue) && (result == 0))
		{
			return 1;
		}
		return result;
	}

	void ClearValue();

	void SetValue(void* NewValue) { Value = NewValue; }
	void SetIntValue(int NewValue) { Value = (void*)NewValue; }


	ARGUMENT_T GetType() { return Type; }
	
	void SetType(ARGUMENT_T NewType) { Type = NewType; }

	ScriptArg *Evaluate();

	void Print(FILE *fp);

	ScriptArg();
	ScriptArg(ScriptArg *pFrom);
	~ScriptArg();

	void Load(FILE *fp);
	void Save(FILE *fp);

	Creature *GetCreature();

	void UnsetCreature();
	void SetCreature();

	bool operator == (ScriptArg& CompArg);
	int operator = (ScriptArg& OtherArg);
};


class ScriptBlock
{
protected:
	int NumArgs;
	ScriptArg *ArgList;
	ScriptArg Value;
	ScriptBlock *pNext;
	ScriptBlock *pPrev;

public:
	ScriptArg *GetValue() { return &Value; }

	ScriptArg* GetArg(int Num) { PTD_ASSERT(Num >= 0); PTD_ASSERT(Num < NumArgs); return &ArgList[Num]; }

	void Save(FILE *fp);

	void Load(FILE *fp);

	void Import(FILE *fp);
	void Import(const char *FileName);
	
	void Export(FILE *fp);

	ScriptArg *Process();

	ScriptBlock *FindLabel(const char *Label);

	void UnsetCreatures();
	void SetCreatures();

	ScriptBlock();
	~ScriptBlock();
};

class ScriptManager
{
protected:
	ScriptBlock *BlockAvailable[8]; //1, 2, 4, 8, 16, 32, 64, 128
	ScriptBlock *BlockUsed[8];//1, 2, 4, 8, 16, 32, 64, 128

public:
	ScriptBlock *GetBlock(int Size);
	void AddBlock(int Size);
	void FreeBlock(ScriptBlock *);

	ScriptManager();
	~ScriptManager();


};

extern ScriptBlock *ScriptContextBlock;
extern ZSWindow *ScriptContextWindow;
extern Flags PreludeFlags;
extern int G_NumBlocks;
extern int G_NumArgs;


#endif
