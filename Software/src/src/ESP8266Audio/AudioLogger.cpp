
#include "AudioLogger.h"

#ifdef HAVE_AUDIO_LOGGER

DevNullOut silencedLogger;
Print* audioLogger = &silencedLogger;

#endif
