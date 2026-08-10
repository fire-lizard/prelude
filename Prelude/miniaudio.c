#include "stb_vorbis.h"

#ifdef __APPLE__
// miniaudio dlopen()s "CoreAudio.framework/CoreAudio" & co. by relative path, which dyld no
// longer resolves -> Core Audio init fails and the engine silently falls back to a Null device
// (no sound, no error). Link the frameworks instead; CMake adds them on APPLE.
#define MA_NO_RUNTIME_LINKING
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
