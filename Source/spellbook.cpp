#include "spellbook.h"
#include <assert.h>

#ifdef __linux__
#include "linux_aux_wrapper.h"
#endif

char Spellbook::SpellbookFieldNames[96*32];
Spellbook *Spellbook::pFirst = NULL;
int Spellbook::NumSpellbooks = 0;


Spellbook::Spellbook()
{
	NumSpellbooks++;

}

Spellbook::~Spellbook()
{
	NumSpellbooks--;
	if(!NumSpellbooks)
	{
		DestroyFieldNames();
	}
}

void LoadSpellbooks(FILE *fp)
{
	Spellbook *pSpellbook, *pNextSpellbook;

	PTD_ASSERT(fp);

	pSpellbook = new Spellbook;
	pSpellbook->SetFirst((Thing *)pSpellbook);

	pSpellbook->LoadFieldNames(fp,Spellbook::SpellbookFieldNames);

	char c;

	while(TRUE)
	{
		pSpellbook->LoadData(fp);
		c = (char)fgetc(fp);
		if(feof(fp) || c == THING_SEPERATION_CHARACTER)
		{
			break;
		}
		else
		{
#ifdef __linux__
			fseek1(fp, -1, 1);
#elif _WIN32
			fseek(fp, -1, 1);
#endif
			pNextSpellbook = new Spellbook;
			pNextSpellbook->SetFieldNames(pSpellbook->GetFieldNames());
			pSpellbook->SetNext(pNextSpellbook);
			pNextSpellbook->SetNumFields(pSpellbook->GetNumFields());
			pSpellbook = pNextSpellbook;
		}
	}

	return;
}


void SaveSpellbooks(FILE *fp)
{
	PTD_ASSERT(fp);

	Spellbook *pSpellbook;

	pSpellbook = (Spellbook *)Spellbook::GetFirst();

	pSpellbook->SaveFieldNames(fp);

	while(pSpellbook)
	{
		pSpellbook->SaveData(fp);
		pSpellbook = (Spellbook *)pSpellbook->GetNext();
	}
	return;
}

void DeleteSpellbooks()
{
	Spellbook *pSpellbook, *pToDelete;

	//get the first Spellbook
	pToDelete = pSpellbook = (Spellbook *)Spellbook::GetFirst();

	while(pSpellbook)
	{
		pToDelete = pSpellbook;
		pSpellbook = (Spellbook *)pSpellbook->GetNext();
		delete pToDelete;
	}

	//GOT_HERE("Deleted Spellbooks",NULL);
	return;
}