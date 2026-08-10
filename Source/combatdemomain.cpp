//version 0.72
// fixed combat event calling switching game out of combat mode
// integrated starting event
//version 0.73
// reintegrated spellcasting
//to be done
// fix saving and loading games

// Don't use MFC for this project
#define WIN32_LEAN_AND_MEAN 

//basic definitions
#include "defs.h"

#include <string>
#ifdef _WIN32
#include <DbgHelp.h>
#include <wincrypt.h>
#endif
#ifdef __linux__
#include <sys/socket.h>
#include <linux/if_alg.h>
#endif

//copyprotection
#include "registration.h"

//main window
#include "mainwindow.h"
#include "zsbutton.h"

#include <time.h>
#include <assert.h>
#include <stdarg.h>

#include "ZSEngine.h"
#include "ZSutilities.h"
#include "creatures.h"
#include "items.h"
#include "zsdescribe.h"
#include "spells.h"
#include "spellbook.h"
#include "Minimap.h"
#include "equipobject.h"
#include "World.h"
#include "script.h"
#include "scriptfuncs.h"
#include "ZSText.h"
#include "party.h"
#include "StartScreen.h"
#include "CreatePartyWin.h"
#include "ZSOptions.h"
#include "events.h"
#include "ZSSaveLoad.h"
#include "ZSCutScene.h"
#include "zsbutton.h"
#include "CombatManager.h"
#include "path.h"
#include "blood.h"
#include "ZSparticle.h"
#include "ZSTalk.h"
#include "ZSHelpWin.h"
#include "ZSRest.h"
#include "ZSOptions.h"
#include "ZSaskwin.h"
#include "mapwin.h"
#include "journal.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <regex.h>
#endif
#ifdef __linux__
#include <openssl/sha.h>
#endif
#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#endif


#define IDC_VERSION_NUMBER		8765

#define MASTER_ITEM_FILE		"items.txt"
#define MASTER_CREATURE_FILE	"creatures.txt"

HWND hMainWindow;
ZSMainWindow *pMain = NULL;

ZSFont *PapyrusParchment;

void OnExit(void);

#ifdef _WIN32
LONG WINAPI PreludeUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* a_pExceptionInfo);

std::string HashBuffer(const void* pBuffer, size_t len)
{
	HCRYPTPROV hProv;
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0)) {
		return "";
	}

	HCRYPTHASH hHash;
	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
		return "";
	}

	if (!CryptHashData(hHash, (const BYTE*)pBuffer, len, 0)) {
		return "";
	}

	DWORD dwCount = sizeof(DWORD);
	DWORD dwHashLen;
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&dwHashLen, &dwCount, 0)) {
		return "";
	}
	PTD_ASSERT(dwHashLen == 20);

	uint8_t hash[20];

	if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &dwHashLen, 0)) {
		return "";
	}

	char result[40 + 1];
	for (size_t i = 0; i < 20; ++i)
	{
		sprintf(result + i * 2, "%02x", hash[i]);
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	return std::string(result, 40);
}

#elif __linux__

std::string HashBuffer(const void* pBuffer, size_t len)
{
	int s = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (s < 0)
		return "";

    sockaddr_alg sa;
    memset(&sa, 0, sizeof(sa));
    sa.salg_family = AF_ALG;
    strcpy((char*)sa.salg_type, "hash");
    strcpy((char*)sa.salg_name, "sha1");
    if (bind(s, (const sockaddr*)&sa, sizeof(sa)) < 0)
    	return "";

    int fd = accept(s, NULL, 0);
    if (fd < 0)
		return "";

	ssize_t r = send(fd, pBuffer, len, MSG_MORE);
	if (r < 0)
		return "";
	if ((size_t)r != len)
		return "";

	uint8_t hash[20];
	r = read(fd, hash, 20);
	close(fd);
	close(s);

	char result[40 + 1];
	for (size_t i = 0; i < 20; ++i)
		sprintf(result + 2 * i, "%02x", (unsigned)hash[i]);
	return result;
}

#elif __APPLE__

std::string HashBuffer(const void* pBuffer, size_t len)
{
	unsigned char hash[CC_SHA1_DIGEST_LENGTH];
	CC_SHA1(pBuffer, (CC_LONG)len, hash);

	char result[40 + 1];
	for (size_t i = 0; i < 20; ++i)
		sprintf(result + 2 * i, "%02x", (unsigned)hash[i]);
	return std::string(result, 40);
}

#else

std::string HashBuffer(const void* pBuffer, size_t len)
{
	// TODO
	return "";
}

#endif

std::string HashFile(const char* path)
{
	FILE* fp = fopen(path, "rb");
	PTD_ASSERT(fp);

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	void* pBuffer = malloc(size);
	PTD_ASSERT(pBuffer);
	fread(pBuffer, size, 1, fp);

	std::string result = HashBuffer(pBuffer, size);

	free(pBuffer);

	fclose(fp);
	return result;
}

#if defined(__linux__) || defined(__APPLE__)
void ClearHashedEvents() {
	DIR *d;
	regex_t    re;
	int st = regcomp(&re, "events\\..*\\.bin$", REG_EXTENDED | REG_NOSUB);

	struct dirent *dire;
	d = opendir(".");
	char target_name[1024] = { 0, };
	if (d) {
		while ((dire = readdir(d)) != NULL) {
			if (!regexec(&re, dire->d_name, 0, 0, 0)) {
				st = remove(dire->d_name);
			}
		}
	}
}
#else
void ClearHashedEvents() {
	HANDLE h;
	WIN32_FIND_DATA fd;

	h = FindFirstFile(".\\events.*.bin", &fd);
	
	if (h == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		debug_info("hashed %s", fd.cFileName);
		DeleteFile(fd.cFileName);
	}  while (FindNextFile(h, &fd));

	FindClose(h);
}
#endif

void LoadEvents()
{
	DEBUG_INFO("Loading Events\n");

	std::string eventsTxtSha1 = HashFile("events.txt");
	if (!eventsTxtSha1.empty())
	{
		char eventsBinPath[512];
		snprintf(eventsBinPath, sizeof(eventsBinPath), "events.%s.bin", eventsTxtSha1.c_str());
		FILE* fp = fopen(eventsBinPath, "rb");
		if (fp)
		{
			fclose(fp);
			PreludeEvents.LoadEvents(eventsBinPath);
		}
		else
		{
			ClearHashedEvents();
			PreludeEvents.ImportEvents("events.txt");
			PreludeEvents.SaveEvents(eventsBinPath);
		}
	}
	else
	{
		ClearHashedEvents();
		PreludeEvents.ImportEvents("events.txt");
		PreludeEvents.SaveEvents("events.bin");
	}
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow)
{
#elif defined(__linux__) || defined(__APPLE__)
int main(int argc, char * argv[]) {
	HINSTANCE hinstance = (HINSTANCE)1;
#endif

#ifdef _WIN32
	BOOL ok = SymInitialize(GetCurrentProcess(), NULL, TRUE);
	if (!IsDebuggerPresent())
		SetUnhandledExceptionFilter(&PreludeUnhandledExceptionFilter);
#endif
	
	INIT_DEBUG();

	debug_info("Version %i", VERSION_NUMBER);

	//uncomment for demo version
	///GetHardwareID();
	//etRegisteredKey();

	//comment out for demo version
	Register();
	
	FillDistanceTable();

	srand((unsigned int)time(NULL));
	
	if(atexit(OnExit))
	{
		DEBUG_INFO("failed to register exit normal function\n");
		exit(1);
	}

	//Chunk::InitTerrain();
	
	LoadFuncs();

	D3DXInitialize();
		
	Engine = new ZSEngine;
	debug_info("engine at %X %i", Engine,sizeof(ZSEngine));
	DEBUG_INFO("Engine init\n");
	
	Engine->Init(hinstance);

	PapyrusParchment = new ZSFont(Engine->Graphics()->GetFontEngine(), "Papyrus.ddf", TEXTCOLOR(250, 250, 250));

	Engine->Graphics()->SetFont(PapyrusParchment);
	
	Engine->Graphics()->CreateProjectionMatrix(8.5f, 8.5f, VIEW_DEPTH*4);

	Engine->Graphics()->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, TRUE );  
	Engine->Graphics()->SetRenderState( D3DRENDERSTATE_ALPHAREF, 0x024 );  
	Engine->Graphics()->SetRenderState( D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	Engine->Graphics()->GetD3D()->SetTextureStageState(0,D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Engine->Graphics()->GetD3D()->SetTextureStageState(0,D3DTSS_MIPFILTER, D3DTFP_LINEAR);
	Engine->Graphics()->GetD3D()->SetTextureStageState(0,D3DTSS_ADDRESS,D3DTADDRESS_WRAP);

	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
#ifdef OPENGL
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_AMBIENT, RGB_MAKE(32, 32, 32).dword());
#else
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_AMBIENT, RGB_MAKE(32,32,32)); 
#endif
	Engine->Graphics()->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);    
	//Engine->Graphics()->SetRenderState(D3DRENDERSTATE_ZBIAS,12);

	//fill the primary and back buffers to 0
	Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	LPDIRECTDRAWSURFACE7 LogoSurface, ProgressSurface;
	
	RECT rLogoTo;
	RECT rProgressTo;

	rLogoTo.left = (Engine->Graphics()->GetWidth() - 640) / 2;
	rLogoTo.right = rLogoTo.left + 640;
	rLogoTo.top = 0;
	rLogoTo.bottom = 69;

	rProgressTo.left = 24;
	rProgressTo.right = 152;
	rProgressTo.top = 128;
	rProgressTo.bottom = 128 + 128;

	LogoSurface = Engine->Graphics()->CreateSurfaceFromFile("logobanner.bmp",640,69,0, -1);

	ProgressSurface = Engine->Graphics()->CreateSurfaceFromFile("loadtextures.bmp",128,128,0, -1);

//	ShowWindow(hMainWindow, SW_SHOW);
	
	opengl_show_cursor(FALSE);

	//create the progress window
	ZSText *pProgressText;
	pProgressText = new ZSText(-5, 27, 104, "Textures");
	pProgressText->SetTextColor(TEXT_WHITE);
	pProgressText->Show();
	pProgressText->Draw();
	Engine->Graphics()->GetBBuffer()->Blt(&rLogoTo, LogoSurface, NULL, NULL, NULL);
	Engine->Graphics()->GetBBuffer()->Blt(&rProgressTo, ProgressSurface, NULL, NULL, NULL);
	
	Engine->Graphics()->Flip();
	Engine->Graphics()->GetBBuffer()->Blt(NULL, Engine->Graphics()->GetPrimay(),NULL,NULL,NULL);

	FILE *fp;
	fp = SafeFileOpen("gui.ini","rt");
	PTD_ASSERT(fp);
	
	SeekTo(fp,"TEXTUREREDUCTION");

	ZSTexture::SetReduction((BOOL)GetInt(fp));
	fclose(fp);
	
	Engine->LoadTextures();
	Engine->Graphics()->SetUpCircles();

	BOOL DrawWorld;
	
	fp = SafeFileOpen("gui.ini","rt");
	PTD_ASSERT(fp);

	SeekTo(fp,"DRAWWORLD");
	DrawWorld = (BOOL)GetInt(fp);

	//pProgressText->SetText("Initiating World");
	//pProgressText->Draw();

	//Engine->Graphics()->Flip();
	//Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	//create base world
	int Temp;
	SeekTo(fp, "SOUND");
	Temp = GetInt(fp);
	if(Temp) 
		Engine->Sound()->SetFX(TRUE);
	else
		Engine->Sound()->SetFX(FALSE);
	
	SeekTo(fp, "SOUNDVOLUME");
	Temp = GetInt(fp);
	Engine->Sound()->SetFXVolume(Temp);
	
	
	SeekTo(fp, "MUSIC");
	Temp = GetInt(fp);
	if(Temp) 
		Engine->Sound()->SetMusic(TRUE);
	else
		Engine->Sound()->SetMusic(FALSE);
	
	SeekTo(fp, "MUSICVOLUME");
	Temp = GetInt(fp);
	Engine->Sound()->SetMusicVolume(Temp);
	
	if(Engine->Sound()->MusicIsOn())
	{
		Engine->Sound()->PlayMusic("god");
	}



	PreludeWorld = new World;
	
	SeekTo(fp, "AUTOSAVERATE");
	PreludeWorld->SetAutosaveRate(GetInt(fp));
	
	pMain = new ZSMainWindow;	
	pMain->SetText("Main Window");
	pMain->SetDrawWorld(DrawWorld);

	float fBias = 0.0f;
	int fbtemp;
	
	SeekTo(fp, "MIPMAPLODBIAS	");
	fbtemp = GetInt(fp);
	
	fBias = (float)fbtemp / 100.0f;
	Engine->Graphics()->GetD3D()->SetTextureStageState( 0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&fBias)) );
	
	fclose(fp);

//	if(DrawWorld)
//	{
	
//	Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	pProgressText->Move(128 + 80,0);
	pProgressText->SetText("Meshes");
	pProgressText->Draw();
	
	ProgressSurface->Release();

	rProgressTo.left += 128 + 80;
	rProgressTo.right += 128 + 80;
	rProgressTo.bottom += 0;
	rProgressTo.top += 0;
	ProgressSurface = Engine->Graphics()->CreateSurfaceFromFile("loadmeshes.bmp",128,128,0, -1);
	Engine->Graphics()->GetBBuffer()->Blt(&rLogoTo, LogoSurface, NULL, NULL, NULL);
	Engine->Graphics()->GetBBuffer()->Blt(&rProgressTo, ProgressSurface, NULL, NULL, NULL);
	
	Engine->Graphics()->Flip();
	Engine->Graphics()->GetBBuffer()->Blt(NULL, Engine->Graphics()->GetPrimay(),NULL,NULL,NULL);

	fp = SafeFileOpen("mesh.bin","rb");
	if(fp)
	{
		fclose(fp);
		Engine->LoadMeshes("mesh.bin");
	}
	else
	{
		Engine->ImportMeshes();
	}

//	}
//	else
//	{
//		Engine->LoadMeshes("mesh.bin");
//	}
//	Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	DEBUG_INFO("about to load items\n");

	pProgressText->Move(128 + 80, 0);
	pProgressText->SetText("Items");
	pProgressText->Draw();
	rProgressTo.left += 128 + 80;
	rProgressTo.right += 128 + 80;
	rProgressTo.bottom += 0;
	rProgressTo.top += 0;
	ProgressSurface->Release();
	ProgressSurface = Engine->Graphics()->CreateSurfaceFromFile("loaditems.bmp",128,128,0, -1);
	Engine->Graphics()->GetBBuffer()->Blt(&rLogoTo, LogoSurface, NULL, NULL, NULL);
	Engine->Graphics()->GetBBuffer()->Blt(&rProgressTo, ProgressSurface, NULL, NULL, NULL);
	
	Engine->Graphics()->Flip();
	Engine->Graphics()->GetBBuffer()->Blt(NULL, Engine->Graphics()->GetPrimay(),NULL,NULL,NULL);

	fp = fopen("items.bin","rb");
	if(!fp)
	{
		fp = SafeFileOpen(MASTER_ITEM_FILE,"rt");
		PTD_ASSERT(fp);
		fseek(fp,0,0);
		LoadItems(fp);
		fclose(fp);
		fp = SafeFileOpen("items.bin","wb");
		SaveBinItems(fp);
	}
	else
	{
		LoadBinItems(fp);
	}
	fclose(fp);

	DEBUG_INFO("Loading Spells\n");

	PreludeSpells.Init();
	PreludeSpells.Load("newspells.bin");


	pProgressText->Move(128 + 80, 0);
	pProgressText->SetText("Creatures");
	pProgressText->Draw();
	rProgressTo.left += 128 + 80;
	rProgressTo.right += 128 + 80;
	rProgressTo.bottom += 0;
	rProgressTo.top += 0;
	ProgressSurface->Release();
	ProgressSurface = Engine->Graphics()->CreateSurfaceFromFile("loadmonsters.bmp",128,128,0, -1);
	Engine->Graphics()->GetBBuffer()->Blt(&rLogoTo, LogoSurface, NULL, NULL, NULL);
	Engine->Graphics()->GetBBuffer()->Blt(&rProgressTo, ProgressSurface, NULL, NULL, NULL);

	Engine->Graphics()->Flip();
	Engine->Graphics()->GetBBuffer()->Blt(NULL, Engine->Graphics()->GetPrimay(),NULL,NULL,NULL);

	DEBUG_INFO("About to load Creatures\n");

	fp = SafeFileOpen("creatures.bin","rb");
	if(!fp)
	{
		fp = SafeFileOpen(MASTER_CREATURE_FILE,"rt");
		PTD_ASSERT(fp);
		//start at beginning of file
		fseek(fp,0,0);
		LoadCreatures(fp);
		fclose(fp);
		fp = SafeFileOpen("creatures.bin","wb");
		SaveBinCreatures(fp);
	}
	else
	{
		LoadBinCreatures(fp);
	}

	ProgressSurface->Release();
	LogoSurface->Release();

	fclose(fp);
	
//EquipAllCreatures;
	Creature *pCreature;
	pCreature = Creature::GetFirst();
	while(pCreature)
	{
		pCreature->ReEquip();
		pCreature = (Creature *)pCreature->GetNext();
	}

//	PreludeParty.LoadDefaultParty("Default");

//	pProgressText->SetText("Creating Spells");
//	pProgressText->Draw();
//	Engine->Graphics()->Flip();
//	Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	
	FILE *fpValley;
	SetCurrentDirectory(".\\Areas");
	fpValley = SafeFileOpen("valley.bin","rb");
	if(!fpValley)
	{
		PreludeWorld->BringUpToDate();
	}
	else
	{
		SetCurrentDirectory(Engine->GetRootDirectory());
		fclose(fpValley);
		PreludeWorld->Load("worldbase.bin");
	}
	


	SetCurrentDirectory(Engine->GetRootDirectory());


	Engine->Graphics()->GetD3D()->BeginScene();
	fp = SafeFileOpen("gui.ini","rt");

	SeekTo(fp,"FILTERING");

	if(GetInt(fp))
	{
		Engine->Graphics()->SetFilterState(FILTER_BOTH);
	}
	else
	{
		Engine->Graphics()->SetFilterState(FILTER_NONE);
	}
	fclose(fp);

	
	Engine->Graphics()->GetD3D()->EndScene();

	fp = SafeFileOpen("gui.ini","rt");
	PTD_ASSERT(fp);

	SeekTo(fp,"[EDITTING]");
	SeekTo(fp,"Enabled:");
	int EditEnable;
	EditEnable = GetInt(fp);

	if(EditEnable)
	{
		PreludeWorld->SetEdittingEnabled(TRUE);
	}
	else
	{
		PreludeWorld->SetEdittingEnabled(FALSE);
	}

	fclose(fp);

	BOOL Test;

	fp = SafeFileOpen("gui.ini","rt");
	PTD_ASSERT(fp);

	SeekTo(fp,"DIFFICULTY");
	int DiffLevel = GetInt(fp);
	PreludeWorld->SetDifficulty(DiffLevel);
	
	SeekTo(fp,"XPCONFIRM");
	Test = (BOOL)GetInt(fp);
	PreludeWorld->SetXPConfirm(Test);
	
	SeekTo(fp,"FULLBODYSELECT");
	Test = (BOOL)GetInt(fp);
	PreludeWorld->SetFullSelect(Test);
	
	SeekTo(fp,"HIGHLIGHTINFO");
	Test = (BOOL)GetInt(fp);
	PreludeWorld->SetHighlightInfo(Test);
	
	SeekTo(fp,"VERBOSECOMBAT");
	Test = (BOOL)GetInt(fp);
	PreludeWorld->SetVerboseCombat(Test);

	fclose(fp);

	fp = SafeFileOpen("gui.ini","rt");
	PTD_ASSERT(fp);

	int Shadowed;
	SeekTo(fp,"SHADOWS");
	Shadowed = (BOOL)GetInt(fp);
	
	if(Shadowed)
		PreludeWorld->SetDrawShadows(TRUE);
	else
		PreludeWorld->SetDrawShadows(FALSE);

	
	fclose(fp);

	Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);
	//pProgressText->SetText("Loading Events");
	//pProgressText->Draw();
	//Engine->Graphics()->Flip();
	//Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),0,NULL);

	LoadEvents();

	delete pProgressText;

//*************************
	if(!DrawWorld)
		Valley->GenerateBase(NULL);
//*************************

//	ZSText *pText;
//
//	pText = new ZSText(IDC_VERSION_NUMBER, 0, 0, VERSION_NUMBER);
//	pMain->AddChild(pText);
//	pText->Show();

	pMain->Show();
	
//test blood
	DEBUG_INFO("Testing Blood....\n");

	Blood *pBlood;
	pBlood = new Blood(1.0f, NULL);

	if(!pBlood->GetSys()->TestDraw())
	{
		DEBUG_INFO("Blood Disabled\n");
		Blood::DisableBlood();
	}
	else
	{
		DEBUG_INFO("Blood Enabled\n");
	}

	delete pBlood;

	DEBUG_INFO("Initting terrain info\n");

	Chunk::InitTerrain();
	

	// enter main event loop

	PreludeStartScreen *pStart;
	CreatePartyWin	*pCreateParty;
	ZSOptionWin *pOptions;

	int Result = START_RESULT_NONE;
	int Created = FALSE;

	BOOL Init = TRUE;
	
	while(Result != START_RESULT_QUIT)
	{
		pMain->SetState(WINDOW_STATE_NORMAL);
		Created = FALSE;
		pStart = new PreludeStartScreen;
		pStart->Show();

		pMain->AddTopChild(pStart);

		pMain->SetDrawWorld(FALSE);

		// ponytail: TEST HOOK - PTD_AUTOLOAD=<save.gam> skips the menus and drops
		// straight into the world, so the in-game renderer and combat can be
		// reproduced over ssh without driving the GUI. Remove before shipping.
		const char * ptdAutoload = getenv("PTD_AUTOLOAD");
		if (ptdAutoload && Init)
		{
			pMain->RemoveChild(pStart);
			PreludeWorld->LoadGame(ptdAutoload);
			PreludeWorld->SetGameState(GAME_STATE_NORMAL);
			pMain->SetDrawWorld(DrawWorld);
			pMain->Show();
			ClearDescribe();
			Describe(VERSION_NUMBER);
			pMain->GoModal();
			Init = FALSE;
			break;
		}

		pMain->SetFocus(pStart);
		Result = pStart->GoModal();
		pStart->ReleaseFocus();

		pMain->RemoveChild(pStart);
		if(Result == START_RESULT_OPTIONS)
		{
			if(!pMain->GetChild(OPTIONS_ID))
			{
				pOptions = new ZSOptionWin(OPTIONS_ID);
				pMain->AddTopChild(pOptions);
			}
			pOptions->Show();
			pOptions->SetFocus(pOptions);
			pOptions->GoModal();
			pOptions->ReleaseFocus();
			pMain->RemoveChild(pOptions);
			
		}

		int MemberNum;
				
		if(Result == START_RESULT_NEW)
		{
		//intro cut scene;
			if(!Init)
			{
				//clear out old info
				ClearStack();

				PreludeEvents.Clear();
				PreludeFlags.Clear();

				if(PreludeWorld->GetCombat())
					PreludeWorld->GetCombat()->Kill();
					
				DeleteCreatures();
			}	
			
			char IntroDir[256];
			//get the maindirectory
			strcpy(IntroDir, Engine->GetRootDirectory());
			strcat(IntroDir, "\\Intro");

			DEBUG_INFO("switching to Intro directory\n");

			//switch to the Texture directory
			SetCurrentDirectory(IntroDir);

			ZSCutScene *pCutScene;
			pCutScene = new ZSCutScene;
			pMain->AddChild(pCutScene);
			fp = SafeFileOpen("introscene.txt","rt");
			pCutScene->Load(fp);
			fclose(fp);

			pCutScene->Show();
			pCutScene->SetFocus(pCutScene);
			pCutScene->GoModal();
			
			pCutScene->ReleaseFocus();

			SetCurrentDirectory(Engine->GetRootDirectory());

			pMain->RemoveChild(pCutScene);

			if(!Init)
			{
				if(PreludeWorld)
				{
					if(PreludeWorld->GetCombat())
						PreludeWorld->GetCombat()->Kill();
					delete PreludeWorld;
					PreludeWorld = new World;
				}
				//clear out addlocations
				PreludeParty.ClearJournal();
				PreludeParty.ClearLocations();
								
				DEBUG_INFO("About to load Creatures\n");
				fp = SafeFileOpen("creatures.bin","rb");
				LoadBinCreatures(fp);
				fclose(fp);

			//EquipAllCreatures;
				Creature *pCreature;
				pCreature = Creature::GetFirst();
				while(pCreature)
				{
					pCreature->ReEquip();
					pCreature = (Creature *)pCreature->GetNext();
				}

				PreludeWorld->Load("worldbase.bin");

				DEBUG_INFO("Loading Events\n");

				PreludeEvents.LoadEvents("events.bin");
				Init = TRUE;
			}
			
			pCreateParty = new CreatePartyWin;
			ZSWindow *pChild;
			pChild = pMain->GetChild();

			while(pChild)
			{
				pChild->Hide();
				pChild = pChild->GetSibling();
			}
				
			pCreateParty->Show();
			pMain->AddTopChild(pCreateParty);

			if(Engine->Sound()->MusicIsOn())
			{
				Engine->Sound()->PlayMusic("god");
			}

			pCreateParty->SetFocus(pCreateParty);
			Created = pCreateParty->GoModal();
			pCreateParty->ReleaseFocus();

			pMain->RemoveChild(pCreateParty);
			pChild = pMain->GetChild(CREATE_CHARACTER_ID);
			if(pChild)
			{
				pMain->RemoveChild(pChild);
			}

			pChild = pMain->GetChild();
			
			while(pChild)
			{
				pChild->Show();
				pChild = pChild->GetSibling();
			}
			
			
			if(Created)
			{
				int n;
				for(n = 0; n < PreludeWorld->GetNumAreas(); n++)
				{
					PreludeWorld->GetArea(n)->ClearDynamic(NULL, OBJECT_CREATURE);
				}

				Creature::PlaceByLocator();
				
				for(MemberNum = 0; MemberNum < PreludeParty.GetNumMembers(); MemberNum++)
				{
					float xy;
					xy = 1200.5 + (float)MemberNum;
					PreludeParty.GetMember(MemberNum)->SetPosition(xy,xy,Valley->GetZ(xy,xy));
					PreludeParty.GetMember(MemberNum)->ReEquip();
				
				//	Valley->AddToUpdate(PreludeParty.GetMember(MemberNum));
					PreludeParty.GetMember(MemberNum)->AddToWorld();
				//	PreludeParty.GetMember(MemberNum)->InsertAction(ACTION_IDLE,NULL,NULL);
					PreludeParty.GetMember(MemberNum)->ClearActions();
					PreludeParty.GetMember(MemberNum)->InsertAction(ACTION_USER,NULL,NULL);
					PreludeParty.GetMember(MemberNum)->InsertAction(ACTION_IDLE,NULL,NULL);
					PreludeParty.GetMember(MemberNum)->SetCreated(TRUE);
				}
				PreludeWorld->LookAt(PreludeParty.GetLeader());
				pMain->SetDrawWorld(DrawWorld);
				pMain->Show();
				ClearDescribe();
				Describe(VERSION_NUMBER);
				Describe("Press 'F1' for help");
				PreludeWorld->SetGameState(GAME_STATE_NORMAL);
				PreludeEvents.RunEvent(0);
				Engine->Input()->setFreeCameraEnabled(1);
				pMain->GoModal();
				
				while(PreludeParty.GetMember(0))
				{
					PreludeParty.GetMember(0)->KillActions();
					PreludeParty.RemoveMember(PreludeParty.GetMember(0));
				}
				Init = FALSE;
			}
	
			Result = START_RESULT_NONE;
		}
		
		if(Result == START_RESULT_LOAD)
		{
			LoadWin *pLoadWin;
			pLoadWin = new LoadWin(-1,100,100,500,400);
			pLoadWin->Show();
			pMain->AddTopChild(pLoadWin);
			pMain->SetFocus(pLoadWin);
			
			int Loaded;
			Loaded = pLoadWin->GoModal();

			pLoadWin->ReleaseFocus();

			pMain->RemoveChild(pLoadWin);

			if(Loaded)
			{
				PreludeWorld->SetGameState(GAME_STATE_NORMAL);
				pMain->SetDrawWorld(DrawWorld);
				pMain->Show();
				ClearDescribe();

				Describe(VERSION_NUMBER);
				Describe("Press 'F1' for help");

				pMain->GoModal();
				
				while(PreludeParty.GetMember(0))
				{
					PreludeParty.GetMember(0)->KillActions();
					PreludeParty.RemoveMember(PreludeParty.GetMember(0));
				}
			
				Init = FALSE;
			}

			Result = START_RESULT_NONE;
		}

	}

	pMain->SetState(WINDOW_STATE_DONE);
		
	// end while
	opengl_show_cursor(TRUE);

	// return to Windows like this
	return(0);
}

void OnExit(void)
{
	if(ZSTalkWin::TalkSurface)
		ZSTalkWin::TalkSurface->Release();
	DEBUG_INFO("released talk\n");
	
	if(ZSHelpWin::HelpSurface)
		ZSHelpWin::HelpSurface->Release();
	DEBUG_INFO("released help\n");
	
	if(ZSOptionWin::OptionsSurface)
		ZSOptionWin::OptionsSurface->Release();
	DEBUG_INFO("released options\n");
	
	if(ZSMainOptionsMenu::MainOptionsSurface)
		ZSMainOptionsMenu::MainOptionsSurface->Release();
	DEBUG_INFO("released mainoptions\n");
	
	if(ZSRest::RestSurface)
		ZSRest::RestSurface->Release();
	DEBUG_INFO("released rest\n");
	
	if(ZSAskWin::AskSurface)
		ZSAskWin::AskSurface->Release();
	DEBUG_INFO("released ask\n");
	
	if(MapWin::MapBackground)
		MapWin::MapBackground->Release();
	DEBUG_INFO("released mpaback\n");
	
	if(MapWin::MapSurface)
		MapWin::MapSurface->Release();
	DEBUG_INFO("released map\n");
	
	if(GameWin::GameWinSurface)
		GameWin::GameWinSurface->Release();
	DEBUG_INFO("released game\n");
	
	if(LoadWin::LoadWinSurface)
		LoadWin::LoadWinSurface->Release();
	DEBUG_INFO("released load\n");
	
	if(SaveWin::SaveWinSurface)
		SaveWin::SaveWinSurface->Release();
	DEBUG_INFO("released save\n");

	if(JournalWin::JournalSurface)
		JournalWin::JournalSurface->Release();
	DEBUG_INFO("released journal\n");
	
	ZSButton::ShutDown();

	D3DXUninitialize();

	ClearStack();

	PreludeEvents.Clear();
	PreludeFlags.Clear();
	
	if(PreludeWorld)
	{
		if(PreludeWorld->GetCombat())
			PreludeWorld->GetCombat()->Kill();
		delete PreludeWorld;
		PreludeWorld = NULL;
	}

	DeleteCreatures();
	DeleteItems();
	
	if(Engine)
	{
		//delete Engine;
		Engine = NULL;
	}

	if(PapyrusParchment)
	{
		delete PapyrusParchment;
		PapyrusParchment = NULL;
	}


	//delete windows last

	if(pMain)
	{
		delete pMain;
		pMain = NULL;
	}
	
	ZSWindow::Shutdown();

	Action::ReleaseAll();\
	DEBUG_INFO("Actions cleared\n");

	char blarg[64];
	sprintf(blarg, "End Script Blocks: %i\nargs: %i\n", G_NumBlocks, G_NumArgs);
	DEBUG_INFO(blarg);

	if(ExitErrorMessage)
	{
		MessageBox(NULL, ExitErrorMessage, "Fatal Error", MB_OK | MB_ICONSTOP);
		delete[] ExitErrorMessage;
	}
}

static FILE * dbginternal = NULL;

void debug_info(const char * str, ...) {
	if (!dbginternal) {
		dbginternal = fopen("debug.out", "w");
	}

	va_list args;
	va_start(args, str);
	char line[4196];
	char line2[4196];
	vsprintf(line, str, args);
	if (line[strlen(line) - 1] != '\n') {
		sprintf(line2, "%s\n", line);
	}
	else {
		sprintf(line2, "%s", line);
	}
	if (dbginternal != NULL) {
		fwrite(line2, 1, strlen(line2), dbginternal);
		fflush(dbginternal);
	}
	else
		puts(line2);		
	va_end(args);
}
