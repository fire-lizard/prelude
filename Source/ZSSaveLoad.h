#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "ZSwindow.h"
#include <cstdint>

class GameWin : public ZSWindow
{
private:
	int GameNumber;

public:
	static LPDIRECTDRAWSURFACE7 GameWinSurface;
	int Command(int IDFrom, int Command, int Param);
	
	void SetGameNumber(int n);
	int GetGameNumber() { return GameNumber; }

	GameWin(int ID, int x, int y, int width, int height);

};

typedef struct _GameListWinGameSlot {
	char FileName[128];
	char StoredFileName[64];
	//World::SaveGame writes both of these as 32 bit, so "long" is wrong on LP64
	int32_t Hour;
	int32_t TotalTime;
} GameListWinGameSlot;

//World::SaveGame writes the header as 64 chars + fwrite(&Hour,sizeof(int)) + fwrite(&TotalTime,sizeof(int))
static_assert(sizeof(((GameListWinGameSlot *)0)->Hour) == sizeof(int)
	&& sizeof(((GameListWinGameSlot *)0)->TotalTime) == sizeof(int),
	"GameListWinGameSlot must match the savegame header World::SaveGame writes");

class GameListWin : public ZSWindow
{
private:
	int NumGames;
	int GamesShown;
	int TopGame;
	BOOL Save;

	void SortGames();
	void SortFiles();

	std::vector<GameListWinGameSlot> Preloaded;
public:
	
	BOOL IsSave() { return Save; }
	void SetLoad();
	void SetSave();
	
	void DeleteGame(int num);
	void AddGame(int num);

	void LoadPatternSaves(char * pattern);

	void LoadAllSaves();

	int Command(int IDFrom, int Command, int Param);

	GameListWin(int ID, int x, int y, int width, int height, BOOL DoSave = TRUE);

	BOOL HasPreloadedSlot(unsigned int num) {
		return num > 0 && num <= Preloaded.size();
	}

	GameListWinGameSlot GetPreloadedSlot(int num) {		
		return Preloaded.at(num-1);
	}

};


class LoadWin : public ZSWindow
{
private:

public:
	static LPDIRECTDRAWSURFACE7 LoadWinSurface;
	int Command(int IDFrom, int Command, int Param);

	LoadWin(int ID, int x, int y, int width, int height);

};

class SaveWin : public ZSWindow
{
private:

public:
	static LPDIRECTDRAWSURFACE7 SaveWinSurface;
	int Command(int IDFrom, int Command, int Param);

	SaveWin(int ID, int x, int y, int width, int height);
};



#endif