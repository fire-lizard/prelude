#include "ZSsound.h"
#include "ZSEngine.h"
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include "ZSutilities.h"
#include "file_input.h"
#include "file_output.h"
#include "audio_output.h"
#include "decoder.h"
#include "mpeg_codec.h"
#include "xaudio.h"
#include "World.h"

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
ma_sound* maStream = NULL;
ma_sound* maNextStream = NULL;
ma_sound* maMp3Stream = NULL;
#endif

void SoundEffect::SetVolume(long lVol)
{
	return;
}

void SoundEffect::Load(const char *Filename)
{
	strcpy(Name, Filename);

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	if (Sound)
	{
		ma_sound_uninit(Sound);
		delete Sound;
		Sound = NULL;
	}
	Sound = new ma_sound;
	ma_result result = ma_sound_init_from_file(&Engine->Sound()->MiniaudioEngine, Filename, 0, &Engine->Sound()->MiniaudioFxGroup, NULL, Sound);
	PTD_ASSERT(result == MA_SUCCESS);
#endif	
}

void SoundEffect::Play()
{
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	ma_result result = ma_sound_start(Sound);
	PTD_ASSERT(result == MA_SUCCESS);
#endif
}

void SoundEffect::Stop()
{
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	ma_result result = ma_sound_stop(Sound);
	PTD_ASSERT(result == MA_SUCCESS);
#endif
}

int ZSSoundSystem::PlayMusic(int n)
{
	if(!MusicOn) 
		return FALSE;
	
	StartMusic(Music[n].Name);

	return TRUE;
}

int ZSSoundSystem::PlayMusic(const char *SuiteName)
{
	
	StartMusic(SuiteName);

	return TRUE;
}

int ZSSoundSystem::PlayEffect(int n)
{
	if(!NumFX)
		return FALSE;
	PTD_ASSERT(n < NumFX && n >= 0);
	

	if(FXOn)
		FX[n].Play();

	return TRUE;
}

int ZSSoundSystem::PlayEffect(const char *EffectName)
{
	int n;
	if(!FXOn) return FALSE;
	
	if(!NumFX) return FALSE;

	for(n = 0; n < NumFX; n++)
	{
		if(!strcmp(FX[n].GetName(),EffectName))
		{
			return PlayEffect(n);
		}
	}

	PTD_ASSERT(n < NumFX);
	
	return FALSE;
}

int ZSSoundSystem::Init(HWND hWindow)
{
	int n;
	DEBUG_INFO("Begin Init Sound\n");

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	ma_result result = ma_engine_init(NULL, &MiniaudioEngine);
	if (result != MA_SUCCESS)
		SafeExit("Could not start MiniAudio");

	result = ma_sound_group_init(&MiniaudioEngine, 0, NULL, &MiniaudioFxGroup);
	PTD_ASSERT(result == MA_SUCCESS);
	result = ma_sound_group_init(&MiniaudioEngine, 0, NULL, &MiniaudioMusicGroup);
	PTD_ASSERT(result == MA_SUCCESS);
#endif

	char Root[256];
	char New[256];

#ifdef _WIN32
	GetCurrentDirectory(256,Root);
#elif defined(__linux__) || defined(__APPLE__)
	getcwd(Root, 256);
#endif

	strcpy(New, Root);

	strcat(New,"/Sound/");
	
	SetCurrentDirectory(New);
	
	//open the soundfx file
	
	int sIndex = 0;

	FILE *fp;

	fp = SafeFileOpen(SOUND_INI_FILENAME, "rt");

	SeekTo(fp,"Number:");

	NumFX = GetInt(fp);

	if(NumFX)
	{
		FX = new SoundEffect[NumFX];
	}
	else
	{
		DEBUG_INFO("No Sounds\n");

		SetCurrentDirectory(Root);

		return TRUE;
	}

	char *SoundFileName;

	for(n = 0; n < NumFX; n++)
	{
		SeekTo(fp,"File:");
		SoundFileName = GetStringNoWhite(fp);
		FX[n].Load(SoundFileName);
	}
	
	SeekTo(fp,"NumSuites:");
	NumMusic = GetInt(fp);
	Music = new MusicSuite[NumMusic];

	for(n = 0; n < NumMusic; n++)
	{
		Music[n].Load(fp);
	}

	fclose(fp);
	
	SetCurrentDirectory(Root);
	
	return TRUE;
}

void ZSSoundSystem::ShutDown()
{
	StopMusic();

	if(FX)
	{
		DEBUG_INFO("Deleting FX\n");
		delete[] FX;
	}

	if(Music)
	{
		DEBUG_INFO("Deleting Music\n");
		delete[] Music;
	}

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	PTD_ASSERT(false); // Not called for now
#endif

	DEBUG_INFO("Sound Shut Down Done\n");
}

void MusicSuite::Load(FILE *fp)
{
	SeekTo(fp,"Suite:");
	char *Blarg;
	Blarg = GetStringNoWhite(fp);
	strcpy(Name,Blarg);
	delete[] Blarg;
	SeekTo(fp,"NumFiles:");
	NumFiles = GetInt(fp);
	SeekTo(fp,"PatternLength:");
	PatternLength = GetInt(fp);
	for(int n = 0; n < PatternLength; n++)
	{
		SeekTo(fp,"Segment:");
		PatternData[n][0] = GetInt(fp);
		for(int sn = 0; sn < PatternData[n][0]; sn++)
		{
			PatternData[n][sn+1] = GetInt(fp);
		}
	}
	CurSegment = 0;
}

MusicSuite *ZSSoundSystem::GetSuite(int n)
{
	return &Music[n];
}

MusicSuite *ZSSoundSystem::GetSuite(const char *SuiteName)
{
	int n;
	for(n = 0; n < NumMusic; n++)
	{
		if(!strcmp(SuiteName,Music[n].Name))
		{
			return &Music[n];
		}
	}
	return NULL;

}



void ZSSoundSystem::StartMusic(const char *area)
{
	
	MusicSuite *CurMusic = NULL;

	int n;
	for(n = 0; n < NumMusic; n++)
	{
		if(!strcmp(area,Music[n].Name))
		{
			CurMusic = &Music[n];
			break;
		}
	}

	MusicPlaying = n;

	StopMusic();

	//initialize the music thread and start it going.
	if(CurMusic)
	{
		if(!MusicOn) 
			return;

		pCurrentMusic = CurMusic;
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
		MusicSuite* pMusic = CurMusic;
		pMusic->CurSegment = 0;

		char FileName[256];
		int rn;
		int n;

		n = 1;
		pMusic->CurSegment = n - 1;

		if (pMusic->CurSegment >= pMusic->PatternLength)
		{
			pMusic->CurSegment = 0;
		}

		pMusic->CurSegment = 0;
		snprintf(FileName, sizeof(FileName), "%s/Sound/%s%i.ogg", Engine->GetRootDirectory(), pMusic->Name, n);

		if (maStream)
		{
			ma_sound_uninit(maStream);
			delete maStream;
			maStream = NULL;
		}
		maStream = new ma_sound;
		ma_result result = ma_sound_init_from_file(&Engine->Sound()->MiniaudioEngine, FileName, MA_SOUND_FLAG_STREAM, &Engine->Sound()->MiniaudioMusicGroup, NULL, maStream);
		PTD_ASSERT(result == MA_SUCCESS);

		PTD_ASSERT(maStream);
		result = ma_sound_start(maStream);
		PTD_ASSERT(result == MA_SUCCESS);

		rn = rand() % pMusic->PatternData[pMusic->CurSegment][0];
		rn++;
		n = pMusic->PatternData[pMusic->CurSegment][rn];
		pMusic->CurSegment = n - 1;

		if (pMusic->CurSegment >= pMusic->PatternLength)
		{
			pMusic->CurSegment = 0;
		}

		snprintf(FileName, sizeof(FileName), "%s/Sound/%s%i.ogg", Engine->GetRootDirectory(), pMusic->Name, n);

		if (maNextStream)
		{
			ma_sound_uninit(maNextStream);
			delete maNextStream;
			maNextStream = NULL;
		}
		maNextStream = new ma_sound;
		result = ma_sound_init_from_file(&Engine->Sound()->MiniaudioEngine, FileName, MA_SOUND_FLAG_STREAM, &Engine->Sound()->MiniaudioMusicGroup, NULL, maNextStream);
		PTD_ASSERT(result == MA_SUCCESS);
#else
		CurrentMusicDeadLine = timeGetTime() + 30 * 1000;
#endif
	//	SetThreadPriority(hmusic, THREAD_PRIORITY_BELOW_NORMAL);
	}
	else
	if(area)
	{
		if(!FXOn) 
			return;
		char filename[256];
		snprintf(filename,sizeof(filename),"%s/Sound/%s.mp3",Engine->GetRootDirectory(),area);
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
		if (maMp3Stream)
		{
			ma_sound_uninit(maMp3Stream);
			delete maMp3Stream;
			maMp3Stream = NULL;
		}
		maMp3Stream = new ma_sound;
		ma_result result = ma_sound_init_from_file(&Engine->Sound()->MiniaudioEngine, filename, MA_SOUND_FLAG_STREAM, &Engine->Sound()->MiniaudioMusicGroup, NULL, maMp3Stream);
		PTD_ASSERT(result == MA_SUCCESS);
		result = ma_sound_start(maMp3Stream);
		PTD_ASSERT(result == MA_SUCCESS);
#endif
	}
	else
	{
		DEBUG_INFO("Could not start music or MP3 track.\n");
	}
}

void ZSSoundSystem::StopMusic()
{
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	ma_sound_uninit(maStream);
	delete maStream;
	maStream = NULL;
	ma_sound_uninit(maNextStream);
	delete maNextStream;
	maNextStream = NULL;

	ma_sound_uninit(maMp3Stream);
	delete maMp3Stream;
	maMp3Stream = NULL;
#else
	CurrentMusicDeadLine = (DWORD)-1;
#endif

	pCurrentMusic = NULL;
}


void ZSSoundSystem::SetMusic(BOOL OnOff)
{ 
	MusicOn = OnOff; 
	
	if(!MusicOn)
	{
		StopMusic();
	}


}

void ZSSoundSystem::SetFX(BOOL OnOff) 
{ 
	FXOn = OnOff; 

}

void ZSSoundSystem::SetFXVolume(int NewVolume)
{
	PTD_ASSERT((NewVolume >= 0) && (NewVolume <= 10));
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	float fVolume = float(NewVolume) / 10.f;
	ma_sound_group_set_volume(&MiniaudioFxGroup, fVolume);
#endif
	FXVolume = NewVolume;
}

void ZSSoundSystem::SetMusicVolume(int NewVolume)
{
	PTD_ASSERT((NewVolume >= 0) && (NewVolume <= 10));
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	float fVolume = float(NewVolume) / 10.f;
	ma_sound_group_set_volume(&MiniaudioMusicGroup, fVolume);
#endif
	MusicVolume = NewVolume;
}

void ZSSoundSystem::Update()
{
	if (!pCurrentMusic)
		return;
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	if (!ma_sound_at_end(maStream))
		return;
#else
	PTD_ASSERT(CurrentMusicDeadLine != (DWORD)-1);
	if (timeGetTime() < CurrentMusicDeadLine)
		return;
#endif

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	PTD_ASSERT(maNextStream);
	ma_result result = ma_sound_start(maNextStream);
	PTD_ASSERT(result == MA_SUCCESS);

	if (maStream)
	{
		ma_sound_uninit(maStream);
		delete maStream;
		maStream = NULL;
	}
	maStream = maNextStream;
	maNextStream = NULL;
#endif

	char FileName[256];

	int rn;
	int n;
	int sx;
	int sy;
	D3DVECTOR vScreen;
	char* musicname;

	MusicSuite* pMusic = pCurrentMusic;

	if (PreludeWorld && PreludeWorld->GetGameState() == GAME_STATE_NORMAL)
	{
		vScreen = PreludeWorld->GetCenterScreen();
		sx = (int)vScreen.x;
		sy = (int)vScreen.y;
		musicname = PreludeWorld->GetMusic(sx, sy);
		MusicSuite* pNewMusic = NULL;
		if (musicname)
		{
			if (strcmp(pMusic->Name, musicname))
			{
				pNewMusic = Engine->Sound()->GetSuite(musicname);
			}
		}
		else
		{
			if (strcmp(pMusic->Name, "god"))
			{
				pNewMusic = Engine->Sound()->GetSuite("god");
			}
		}
		if (pNewMusic && pNewMusic != pMusic)
		{
			pMusic = pNewMusic;
			pMusic->CurSegment = 0;
			n = 1;
		}
		else
		{
			rn = rand() % pMusic->PatternData[pMusic->CurSegment][0];
			rn++;
			n = pMusic->PatternData[pMusic->CurSegment][rn];
			pMusic->CurSegment = n - 1;

			if (pMusic->CurSegment >= pMusic->PatternLength)
			{
				pMusic->CurSegment = 0;
			}
		}
	}
	else
	{
		rn = rand() % pMusic->PatternData[pMusic->CurSegment][0];
		rn++;
		n = pMusic->PatternData[pMusic->CurSegment][rn];
		pMusic->CurSegment = n - 1;

		if (pMusic->CurSegment >= pMusic->PatternLength)
		{
			pMusic->CurSegment = 0;
		}
	}

	snprintf(FileName, sizeof(FileName), "%s/Sound/%s%i.ogg", Engine->GetRootDirectory(), pMusic->Name, n);

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	PTD_ASSERT(maNextStream == NULL);
	maNextStream = new ma_sound;

	result = ma_sound_init_from_file(&Engine->Sound()->MiniaudioEngine, FileName, MA_SOUND_FLAG_STREAM, &Engine->Sound()->MiniaudioMusicGroup, NULL, maNextStream);
	PTD_ASSERT(result == MA_SUCCESS);
#else
	CurrentMusicDeadLine = timeGetTime() + 30 * 1000;
#endif
}