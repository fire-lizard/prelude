#ifndef ZSSOUND_H
#define ZSSOUND_H

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef PTD_SOUND_DRIVER_MINIAUDIO
#include "miniaudio.h"
#endif

#if __linux__
#include "linux_aux_wrapper.h"
#endif
#include <stdio.h>


#define SOUND_INI_FILENAME	"sound.ini"
#define MAX_SOUND_NAME		48

class SoundEffect
{
private:
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
	ma_sound* Sound;
#endif
	char Name[MAX_SOUND_NAME];
	

public:

	void Load(const char *Filename);
	void Play();
	void Stop();
	void SetVolume(long lVol);
	char *GetName() { return Name; }

	SoundEffect()
	{
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
		Sound = NULL;
#endif
		Name[0] = '\0';
	}

	~SoundEffect()
	{
#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
		if (Sound)
		{
			ma_sound_uninit(Sound);
			delete Sound;
		}
#endif
	}

};

class MusicSuite
{
public:
	int NumFiles;
	int PatternData[64][16];
	int PatternLength;
	char Name[32];
	int CurSegment;
	void Load(FILE *fp);

};

class ZSSoundSystem
{
private:

	//DirectSound
	SoundEffect *FX;
	MusicSuite *Music;
	MusicSuite *pCurrentMusic;

	BOOL FXOn;
	BOOL MusicOn;

	int FXVolume;
	int MusicVolume;

	int NumFX;
	int NumMusic;

	int MusicPlaying;

#if defined(PTD_SOUND_DRIVER_MINIAUDIO)
public: // TODO find the good abstraction
	ma_engine      MiniaudioEngine;
	ma_sound_group MiniaudioFxGroup;
	ma_sound_group MiniaudioMusicGroup;
#else
	DWORD CurrentMusicDeadLine;
#endif

public:

	ZSSoundSystem()
	{ 
#if !defined(PTD_SOUND_DRIVER_MINIAUDIO)
		CurrentMusicDeadLine = (DWORD)-1;
#endif
		FX = NULL;
		Music = NULL;
		pCurrentMusic = NULL;
		FXOn = TRUE;
		MusicOn = FALSE;
		FXVolume = 10;
		MusicVolume = 10;
		NumFX = 0;
		NumMusic = 0;
		MusicPlaying = 0;
	}

	int PlayMusic(int n);
	int PlayMusic(const char *SuiteName);

	int PlayEffect(int n);
	int PlayEffect(const char *EffectName);

	int Init(HWND hWindow);
	void ShutDown();

	BOOL MusicIsOn() { return MusicOn; }
	BOOL FxAreOn() { return FXOn; }

	void SetMusic(BOOL OnOff);
	void SetFX(BOOL OnOff);

	int GetFXVolume() { return FXVolume; }
	int GetMusicVolume() { return MusicVolume; }
	void SetFXVolume(int NewVolume); 
	void SetMusicVolume(int NewVolume);

	void StartMusic(const char *area);
	void StopMusic();

	MusicSuite *GetSuite(int n);
	MusicSuite *GetSuite(const char *SuiteName);

	int GetMusicPlaying() { return MusicPlaying; }

	void Update();
};


#endif