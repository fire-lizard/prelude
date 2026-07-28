/*
 * Flags.cpp
 */

#include "Flags.h"
#include "defs.h"
#include <assert.h>
#include <ctype.h>


#define FLAG_FILE "flags.txt"

void Flag::Save(FILE *fp)
{
	BOOL NextHere = TRUE;
	BOOL NextNotHere = FALSE;
	fwrite(Name,32,1,fp);
	uint32_t value = (uint32_t)(uintptr_t)Value;
	// We can't serialize meaningfully flags that contain a real pointer and not a glorified int
	// For 32/64 bit reasons, but REALLY for semantic reasons, so let's catch those
	// The common case is ^PartyMember^ which is just used in a transient way and should not be persistent
	// Wo we should fix the script to reset it to NULL when it's not used anymore
	if ((void*)value != Value)
	{
		char tmp[256];
		sprintf(tmp, "Serializing Flag \"%s\" which seems to hold a pointer, please fixup the scripts", Name);
		DEBUG_INFO(tmp);
		value = 0;
		fwrite(&value, sizeof(value), 1, fp);
	}
	else
		fwrite(&value,sizeof(value),1,fp);
	if(pNext)
	{
		fwrite(&NextHere, sizeof(NextHere),1, fp);
		pNext->Save(fp);
	}
	else
	{
		fwrite(&NextNotHere, sizeof(NextNotHere),1,fp);
	}
}

void Flag::Load(FILE *fp)
{
	BOOL NextHere;
	fread(Name,32,1,fp);
	uint32_t value;
	fread(&value,sizeof(value),1,fp);
	Value = (void*)value;
	fread(&NextHere,sizeof(NextHere),1,fp);
	if(NextHere)
	{
		if(!pNext)
			pNext = new Flag;
		pNext->Load(fp);
	}
	else
	{
		if(pNext)
		{
			delete pNext;
			pNext = NULL;
		}
	}	
}

void Flag::Print(FILE *fp)
{
	// See comment in Flag::Save about pointer Flag's
	fprintf(fp,"%s: %i\n", Name, (int)(uintptr_t)Value);
	if(pNext)
	{
		fprintf(fp,"\n   ");
		pNext->Print(fp);
	}
}

Flag *Flags::Get(const char *FlagName)
{
	int BucketNum;

	BucketNum = tolower(FlagName[0]);
	BucketNum -= 'a';

	Flag *pFlag;

	pFlag = &Buckets[BucketNum];

	while(TRUE)
	{
		if(pFlag->Name[0] == '\0')
		{
			strcpy(pFlag->Name,FlagName);
			DEBUG_INFO("FLAG added in Get: ");
			DEBUG_INFO(FlagName);
			DEBUG_INFO("\n");
			return pFlag;
		}
		else
		{
			if(!strcmp(pFlag->Name,FlagName))
			{
				return pFlag;
			}
		}

		if(!pFlag->pNext)
		{
			pFlag->pNext = new Flag;
		}

		pFlag = pFlag->pNext;
	}

	return NULL;
}

int Flags::Kill(char *FlagName)
{
	int BucketNum;

	BucketNum = tolower(FlagName[0]);
	BucketNum -= 'a';

	Flag *pFlag;

	pFlag = &Buckets[BucketNum];

	while(TRUE)
	{
		if(pFlag->Name[0] == '\0')
		{
			return FALSE;
		}
		else
		{
			if(!strcmp(pFlag->Name, FlagName))
			{
				while(pFlag->pNext)
				{
					strcpy(pFlag->Name,pFlag->pNext->Name);
					pFlag->Value = pFlag->pNext->Value;
					pFlag = pFlag->pNext;
				}
				pFlag->Name[0] = '\0';
				pFlag->Value = 0;
				return TRUE;
			}
		}

		if(!pFlag->pNext)
		{
			return FALSE;
		}

		pFlag = pFlag->pNext;
	}

	return FALSE;
}


void Flags::Save(FILE *fp)
{
	PTD_ASSERT(fp);
	for(int n = 0; n < 26; n++)
	{
		Buckets[n].Save(fp);
	}
}

void Flags::Load(FILE *fp)
{
	PTD_ASSERT(fp);
	Clear();
	for(int n = 0; n < 26; n++)
	{
		Buckets[n].Load(fp);
	}

}
	
void Flags::Import()
{


}

void Flags::AddFlag(const char *FlagName)
{



}


void Flags::OutPutDebugInfo(FILE *fp)
{
	for(int n = 0; n < 26; n++)
	{
		Buckets[n].Print(fp);
	}
}

void Flags::Clear()
{
	Flag *pFlag;
	for(int n = 0; n < 26; n++)
	{
		Buckets[n].Name[0] = '\0';
		Buckets[n].Value = 0;
		pFlag = Buckets[n].pNext;
		while(pFlag)
		{
			pFlag->Name[0] = '\0';
			pFlag->Value = 0;
			pFlag = pFlag->pNext;
		}
	}
}

Flags::~Flags()
{
	for(int n = 0; n < 26; n++)
	{
		Buckets[n].Name[0] = '\0';
		Buckets[n].Value = 0;
		if(Buckets[n].pNext)
		{
			delete Buckets[n].pNext;
			Buckets[n].pNext = NULL;
		}
	
	}

}
