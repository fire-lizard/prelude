#include "script.h"
#include "scriptfuncs.h"
#include "ZSutilities.h"
#include "ZSTalk.h"
#include "things.h"
#include "creatures.h"
#include "items.h"
#include <assert.h>
#include "party.h"
#include <ctype.h>

#if defined(__linux__) || defined(__APPLE__)
#include "linux_aux_wrapper.h"
#endif

#define IDC_TALK_WIN		666

int G_NumBlocks = 0;
int G_NumArgs = 0;

ScriptBlock *ScriptContextBlock;
ZSWindow *ScriptContextWindow;
Flags PreludeFlags;


ScriptArg::ScriptArg()
{
	Value = NULL;
	Type = ARG_NONE;
	G_NumArgs++;
}

ScriptArg::~ScriptArg()
{
	if(Value)
	{
		switch(Type)
		{
			case ARG_LABEL:
			case ARG_CHARACTER_LABEL:
			case ARG_STRING:
				delete[] (char *)Value;
				break;
			case ARG_BLOCK:
				delete (ScriptBlock *)Value;
				break;
			default:
				break;
		}
	}
	Value = NULL;
	Type = ARG_NONE;
	G_NumArgs--;
}

void ScriptArg::ClearValue()
{	
	if(Value)
	{
		switch(Type)
		{
			case ARG_LABEL:
			case ARG_CHARACTER_LABEL:
			case ARG_STRING:
				delete[] (char *)Value;
				break;
			case ARG_BLOCK:
				delete (ScriptBlock *)Value;
				break;
			default:
				break;
		}
	}
	Value = NULL;
	Type = ARG_NONE;

}


ScriptArg::ScriptArg(ScriptArg *pFrom)
{
	Type = pFrom->GetType();

	switch(pFrom->Type)
	{
		case ARG_LABEL:
		case ARG_CHARACTER_LABEL:
		case ARG_STRING:
			Value = new char[strlen((char *)pFrom->GetValue()) + 1];
			PTD_ASSERT(Value);
			strcpy((char *)Value,(char *)pFrom->GetValue());
			break;
		case ARG_BLOCK:
			SafeExit("error: attempted to copy Script block\n");
			break;
		default:
			Value = pFrom->GetValue();
			break;
	}
	G_NumArgs++;
}

void ScriptArg::Save(FILE *fp)
{
//write out the typeof arg
	fwrite(&Type,sizeof(Type),1,fp);
	int ID;
	unsigned short Length;
	
	switch(Type)
	{
	case ARG_TERMINATOR:
	case ARG_NONE:
		break;
	case	ARG_FUNC_ID:
	case	ARG_NUMBER:
	{
		uintptr_t castValue = (uintptr_t)Value;
		int32_t valueAsInt= (int32_t)castValue;
		PTD_ASSERT(valueAsInt == castValue);
		fwrite(&valueAsInt,sizeof(valueAsInt),1,fp);
		break;
	}
	case ARG_LABEL:
	case ARG_CHARACTER_LABEL:
	case ARG_STRING:
		Length = (unsigned short) strlen((char *)Value) + 1;
		fwrite(&Length,sizeof(Length),1,fp);
		fwrite(Value,Length-1,1,fp);
		break;
	case ARG_BLOCK:
		((ScriptBlock *)Value)->Save(fp);
		break;
	case ARG_ITEM:
	case ARG_CREATURE:
		ID = ((Thing *)Value)->GetData(INDEX_ID).Value;
		fwrite(&ID,sizeof(ID),1,fp);
		break;
	case ARG_FLAG:
		Length = (unsigned short) strlen(((Flag *)Value)->Name) + 1;
		fwrite(&Length,sizeof(Length),1,fp);
		fwrite(((Flag *)Value)->Name,Length-1,1,fp);
		break;
	default:
		break;
	}
}

void ScriptArg::Load(FILE *fp)
{
//write out the typeof arg
	fread(&Type,sizeof(Type),1,fp);
	int ID;
	char TempString[128];
	unsigned short Length;
	Value = NULL;

	switch(Type)
	{
	case ARG_TERMINATOR:
	case ARG_NONE:
		break;
	case ARG_FUNC_ID:
	case ARG_NUMBER:
	{
		int32_t valueAsInt = 0;
		fread(&valueAsInt,sizeof(valueAsInt),1,fp);
		Value = (void*)(uintptr_t)valueAsInt;
		break;
	}
	case ARG_LABEL:
	case ARG_CHARACTER_LABEL:
	case ARG_STRING:
		fread(&Length,sizeof(Length),1,fp);
		Value = new char[Length];
		PTD_ASSERT(Value);
		fgets((char *)Value,Length,fp);
		break;
	case ARG_BLOCK:
		Value = new ScriptBlock;
		PTD_ASSERT(Value);
		((ScriptBlock *)Value)->Load(fp);
		break;
	case ARG_ITEM:
		fread(&ID,sizeof(ID),1,fp);
		Value = (void *)Thing::Find(Item::GetFirst(),ID);
		if(!Value)
		{
			sprintf(TempString,"Item: %i not found in load\n",ID);
			SafeExit(TempString);
		}
		break;
	case ARG_CREATURE:
		fread(&ID,sizeof(ID),1,fp);
		Value = (void *)Thing::Find(Creature::GetFirst(),ID, 0);
		if(!Value)
		{
			sprintf(TempString,"Creature: %i not found in load\n",ID);
			SafeExit(TempString);
		}
		break;
	case ARG_FLAG:
		fread(&Length,sizeof(Length),1,fp);
		fgets(TempString,Length,fp);
		PTD_ASSERT(Length < 128);
		Value = (void *)PreludeFlags.Get(TempString);
		break;
	default:
		break;
	}
}

bool ScriptArg::operator == (ScriptArg& CompArg)
{
	if(this->Type == CompArg.Type)
	{
		return (this->Value == CompArg.Value);
	}
	else
	{
		if(this->Type >= ARG_CREATURE && CompArg.Type >= ARG_CREATURE)
		{
			return (this->GetCreature() == CompArg.GetCreature());	
		}
	}
	return FALSE;
}

int ScriptArg::operator = (ScriptArg& OtherArg)
{
	Type = OtherArg.GetType();

	switch(Type)
	{
		case ARG_LABEL:
		case ARG_CHARACTER_LABEL:
		case ARG_STRING:
			Value = new char[strlen((char *)OtherArg.GetValue()) + 1];
			PTD_ASSERT(Value);
			strcpy((char *)Value,(char *)OtherArg.GetValue());
			break;
		case ARG_BLOCK:
			SafeExit("error: attempted to copy Script block\n");
			break;
		default:
			Value = OtherArg.GetValue();
			break;
	}
	
	return FALSE;
}


Creature *ScriptArg::GetCreature()
{
	switch(this->Type)
	{
		case ARG_CREATURE:
		case ARG_NUMBER:
			return (Creature *)Value;
		case ARG_LEADER:
			return PreludeParty.GetLeader();
		case ARG_PARTYONE:
			return PreludeParty.GetMember(0);
		case ARG_PARTYTWO:
			return PreludeParty.GetMember(1);
		case ARG_PARTYTHREE:
			return PreludeParty.GetMember(2);
		case ARG_PARTYFOUR:
			return PreludeParty.GetMember(3);
		case ARG_PARTYFIVE:
			return PreludeParty.GetMember(4);
		case ARG_PARTYSIX:
			return PreludeParty.GetMember(5);
		default:
			break;
	}
	return NULL;
}


void ScriptBlock::Save(FILE *fp)
{
	fwrite(&NumArgs,sizeof(NumArgs),1,fp);

	int n;

	for(n = 0; n < NumArgs; n++)
	{
		ArgList[n].Save(fp);
	}
}


void ScriptBlock::Load(FILE *fp)
{
	fread(&NumArgs,sizeof(NumArgs),1,fp);

	ArgList = new ScriptArg[NumArgs];
	PTD_ASSERT(ArgList);
	
	int n;

	for(n = 0; n < NumArgs; n++)
	{
		ArgList[n].Load(fp);
	}

}
void ScriptBlock::Import(const char *FileName)
{
	FILE *fp;
	fp = SafeFileOpen(FileName, "rt");

	//seek to the first paren
	SeekTo(fp, "(");

	Import(fp);

	fclose(fp);
}

void ScriptBlock::Import(FILE *fp)
{
	ScriptArg TempArgs[MAX_ARGS];
	
	NumArgs = 0;

	//look at our next char
	char c = '\0';

	char *TempString;
	Thing *pThing;

	int n;
	
	while(1)
	{
		c = GetChar(fp);
		
		if(c == '(')
		{
			TempArgs[NumArgs].SetType(ARG_BLOCK);
			TempArgs[NumArgs].SetValue(new ScriptBlock);
			if(!TempArgs[NumArgs].GetValue())
			{
				SafeExit("COULD not allocate block!!!!!");
			}
			else
			{
				((ScriptBlock *)TempArgs[NumArgs].GetValue())->Import(fp);
			}
		}	
		else
		if(isalpha(c))
		{
			TempArgs[NumArgs].SetType(ARG_FUNC_ID);
			char *FuncName;
#if defined(__linux__) || defined(__APPLE__)
			fseek1(fp, -1, 1);
#elif _WIN32
			fseek(fp, -1, 1);
#endif
			FuncName = GetPureString(fp);
			TempArgs[NumArgs].SetValue((void *)GetFuncID(FuncName));
			delete[] FuncName;
		}
		else
		if(c == '[')
		{
			TempArgs[NumArgs].SetType(ARG_STRING);
			TempArgs[NumArgs].SetValue(GetString(fp, ']'));
		}
		else
		if(c == '!')
		{
			TempArgs[NumArgs].SetType(ARG_LABEL);
			TempArgs[NumArgs].SetValue(GetString(fp, '!'));
		}
		else
		if(c == '#')
		{
			TempArgs[NumArgs].SetType(ARG_CHARACTER_LABEL);
			TempArgs[NumArgs].SetValue(GetString(fp, '#'));
		}
		else
		if(c == '^')
		{
			TempArgs[NumArgs].SetType(ARG_NUMBER);
			TempString = GetString(fp,'^');
			//check special substitutions
			GetSub(&TempArgs[NumArgs],TempString);
			if(TempArgs[NumArgs].GetType() == ARG_NONE)
			{
				//creatures
				pThing = Thing::Find(Creature::GetFirst(), TempString);
				if(pThing)
				{
					TempArgs[NumArgs].SetValue(pThing);
					TempArgs[NumArgs].SetType(ARG_CREATURE);
				}
				else
				{
					//then items
					pThing = Thing::Find(Item::GetFirst(), TempString);
					if(pThing)
					{
						TempArgs[NumArgs].SetValue(pThing);
						TempArgs[NumArgs].SetType(ARG_ITEM);
					}
				}
				if(!pThing)
				{
					//didn't find in things or creatures
					ConvertToCapitals(TempString);
					
					n = Creature::GetFirst()->GetIndex(TempString);
					if(n != - 1)
					{
						TempArgs[NumArgs].SetValue((void *)n);
						TempArgs[NumArgs].SetType(ARG_NUMBER);
					}
					else
					{
						n = Item::GetFirst()->GetIndex(TempString);
						if(n != -1)
						{
							TempArgs[NumArgs].SetValue((void *)n);
						}
						else
						{
							TempArgs[NumArgs].SetValue((void *)PreludeFlags.Get(TempString));
							TempArgs[NumArgs].SetType(ARG_FLAG);
						}
					}
				}
			}
			if(TempString)
				delete[] TempString;
		}
		else
		if(isdigit(c) || c == '-')
		{
#if defined(__linux__) || defined(__APPLE__)
			fseek1(fp, -1, 1);
#elif _WIN32
			fseek(fp, -1, 1);
#endif
			TempArgs[NumArgs].SetType(ARG_NUMBER);
			TempArgs[NumArgs].SetValue((void *)GetInt(fp));
		}	
		if(c == ')' || feof(fp)) 
		{
			if(feof(fp))
			{
				DEBUG_INFO("Hit eof early when importing script");
			}
			break;
		}
		else
		{
			NumArgs++;
			if(NumArgs >= MAX_ARGS)
			{
				SafeExit("Too many args!\n");
			}
		}
	}

	ArgList = new ScriptArg[NumArgs + 1];
	PTD_ASSERT(ArgList);

	for(n = 0; n < NumArgs; n++)
	{
		memcpy(&ArgList[n],&TempArgs[n],sizeof(ScriptArg));
		TempArgs[n].SetValue(NULL);
		TempArgs[n].SetType(ARG_NONE);
	}
	ArgList[NumArgs].SetValue(NULL);
	ArgList[NumArgs].SetType(ARG_TERMINATOR);
	NumArgs++;
	
	return;
}


ScriptBlock::ScriptBlock()
{
	NumArgs = 0;
	ArgList = NULL;
	G_NumBlocks++;
}

ScriptBlock::~ScriptBlock()
{
	if(ArgList)
	{
		Value.ClearValue();

		delete[] ArgList;
		ArgList = NULL;
	}
	G_NumBlocks--;
}

void ScriptBlock::Export(FILE *fp)
{
	fprintf(fp,"(");
	int n;
	for(n = 0; n < NumArgs; n++)
	{
		ArgList[n].Print(fp);
	}
	fprintf(fp,") \n");
}

ScriptArg *ScriptBlock::Process()
{
	int n;
	
	Value.ClearValue();

	ScriptArg *pArgZero;
	pArgZero = ArgList[0].Evaluate();
	
	if(pArgZero->GetType() == ARG_FUNC_ID)
	{
		CallFunc(ArgList[0].GetIntValue(), &ArgList[1], &Value);
		return &Value;
	}
	if(pArgZero->GetType() == ARG_CHARACTER_LABEL)
	{
		ZSWindow *pWin;
		pWin = new ZSTalkWin(IDC_TALK_WIN,125, 125, 550,350, this);

		pWin->Show();
	
		ZSWindow::GetMain()->AddTopChild(pWin);
	
		pWin->SetFocus(pWin);

		pWin->GoModal();

		pWin->ReleaseFocus();
		pWin->Hide();
		
		ZSWindow::GetMain()->RemoveChild(pWin);
	}
	else
	{
		Value = *pArgZero;
		if(Value.GetType() != ARG_TERMINATOR)
		{
			n = 1;
			while(TRUE)
			{

				if(ArgList[n].Evaluate()->GetType() == ARG_TERMINATOR)
				{
					if(ArgList[n].GetType() != ARG_TERMINATOR)
					{
						Value.SetType(ARG_TERMINATOR);
					}
					break;
				}
				n++;
			}
		}
		else
		{
			if(ArgList[0].GetType() == ARG_TERMINATOR)
			{
				Value.SetType(ARG_NONE);
			}
		}
	}
	return &Value;
}

ScriptArg *ScriptArg::Evaluate()
{
	if(Type == ARG_BLOCK)
	{
		return ((ScriptBlock *)Value)->Process();
	}
	else
	{
		return this;
	}
}



ScriptBlock *ScriptBlock::FindLabel(const char *Label)
{
	int n;
	for(n = 0; n < NumArgs; n++)
	{
		//debug_info("label at %i %X\n", ArgList[n].GetType(),this);
		if(ArgList[n].GetType() == ARG_BLOCK)
		{
			if(((ScriptBlock *)ArgList[n].GetValue())->GetArg(0)->GetType() == ARG_LABEL)
			{
				if(!strcmp((char *)((ScriptBlock *)ArgList[n].GetValue())->GetArg(0)->GetValue(),Label))
				{					
					return (ScriptBlock *)ArgList[n].GetValue();
				}
			}
		}
	}
	return NULL;
}

void ScriptArg::Print(FILE *fp)
{

	char FuncName[32];
	switch(Type)
	{
		case ARG_FUNC_ID:
		{
			int funcId = (int)(uintptr_t)Value; // TODO check
			GetFuncName(funcId,FuncName);
			fprintf(fp,"%s",FuncName);
			break;
		}
		case ARG_LABEL:
			fprintf(fp,"!%s!\n",(char *)Value);
			break;
		case ARG_CHARACTER_LABEL:
			fprintf(fp,"#%s#\n",(char *)Value);
			break;
		case ARG_STRING:
			fprintf(fp," [%s]",(char *)Value);
			break;
		case ARG_NUMBER:
			fprintf(fp," %i",(int)(uintptr_t)Value); // TODO Check
			break;
		case ARG_BLOCK:
			((ScriptBlock *)Value)->Export(fp);
			break;
		case ARG_FLAG:
			fprintf(fp," ^%s^", ((Flag *)Value)->Name);
			break;
		case ARG_ITEM:
		case ARG_CREATURE:
			fprintf(fp," ^%s^", ((Thing *)Value)->GetData(INDEX_NAME).String);
			break;
		case ARG_TERMINATOR:
			break;
		case ARG_PARTY:
			fprintf(fp, "^PARTY^");
			break;
		case ARG_LEADER:
		case ARG_PARTYONE:
		case ARG_PARTYTWO:
		case ARG_PARTYTHREE:
		case ARG_PARTYFOUR:
		case ARG_PARTYFIVE:
		case ARG_PARTYSIX:
			if(GetCreature())
				fprintf(fp," ^%s^", GetCreature()->GetData(INDEX_NAME).String);
			else
				fprintf(fp,"NULL");
			break;
		default:
			fprintf(fp," NULL");
			break;
	}
}

void ScriptArg::UnsetCreature()
{
		int ID;
		char *flagname;
	switch(this->Type)
	{
		case ARG_CREATURE:
			ID = ((Creature *)Value)->GetData(INDEX_ID).Value;
			Value = (void *)ID;
			break;
		case ARG_BLOCK:
			((ScriptBlock *)Value)->UnsetCreatures();
			break;
		case ARG_FLAG:
			flagname = new char[32];
			memcpy(flagname, ((Flag *)Value)->Name, sizeof(char) * 32);
			Value = (void *)flagname;
			break;
		default:
			break;
	}
	return;
}

void ScriptArg::SetCreature()
{
	int ID;
	Flag *pFlag;
	char *flagname;
	switch(this->Type)
	{
		case ARG_CREATURE:
		{
			uintptr_t castValue = (uintptr_t)Value;
			int valueAsInt= (int)castValue;
			PTD_ASSERT(valueAsInt == castValue);
			ID = valueAsInt;
			Value = (void *)Thing::Find(Creature::GetFirst(),ID, 0);
			break;
		}
		case ARG_BLOCK:
			((ScriptBlock *)Value)->SetCreatures();
			break;
		case ARG_FLAG:
			flagname = (char *)Value;
			pFlag = PreludeFlags.Get(flagname);
			delete[] flagname;
			Value = (void *)pFlag;
			break;
		default:
			break;
	}
}

void ScriptBlock::UnsetCreatures()
{
	for(int n = 0; n < NumArgs; n++)
	{
		ArgList[n].UnsetCreature();
	}
}

void ScriptBlock::SetCreatures()
{
	for(int n = 0; n < NumArgs; n++)
	{
		ArgList[n].SetCreature();
	}
}

ScriptBlock *ScriptManager::GetBlock(int Size)
{

	return NULL;
};

void ScriptManager::AddBlock(int Size)
{

};

void ScriptManager::FreeBlock(ScriptBlock *)
{

}

	
ScriptManager::ScriptManager()
{

}

ScriptManager::~ScriptManager()
{

}

