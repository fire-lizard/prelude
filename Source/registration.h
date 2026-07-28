#ifndef REGISTRATION_H
#define REGISTRATION_H

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include "linux_aux_wrapper.h"
#endif

#define LockKey "abcd0123efgh4567"
#define HardWareMaskID "qwertasdflkjhzxa"

extern unsigned char HardwareID[];
extern unsigned char RegistrationKey[];

void GetHardwareID();
void GetOldHardwareID();
BOOL ValidateHardwareID(unsigned char *tovalidate);
inline void GenerateKey(unsigned char *Destination, unsigned char *From);
void GetRegisteredKey();
void RegisterKey(unsigned char *toregister);
BOOL ValidateKey(unsigned char *keytovalidate);
void MaskStrings(unsigned char *string, unsigned char *mask, unsigned char *result);

HRESULT Register();


#endif
