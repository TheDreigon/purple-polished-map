#ifndef VERSION_H
#define VERSION_H

#define PROGRAM_VERSION 1,0,0
#ifdef _DEBUG
#define PROGRAM_VERSION_STRING "1.0.0 [DEBUG]"
#else
#define PROGRAM_VERSION_STRING "1.0.0"
#endif

#define PROGRAM_EXE_NAME "purple-polished-map"

#ifdef _WIN32
#define PROGRAM_EXE PROGRAM_EXE_NAME ".exe"
#else
#define PROGRAM_EXE PROGRAM_EXE_NAME
#endif

#define PROGRAM_NAME "Purple Polished Map"

#define PROGRAM_AUTHOR "TheDreigon"

#define CURRENT_YEAR "2026"

#define PROGRAM_SOURCE_URL "https://github.com/TheDreigon/purple-polished-map"

// This program is an unofficial fork of Polished Map
#define ORIGINAL_NAME "Polished Map"

#define ORIGINAL_AUTHOR "Rangi"

#define ORIGINAL_YEAR "2021"

#define ORIGINAL_SOURCE_URL "https://github.com/Rangi42/polished-map"

#endif
