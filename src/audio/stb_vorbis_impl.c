// Single C translation unit that compiles stb_vorbis.
// Kept in its own .c file so the heavy header-only impl
// is compiled once and the rest of the engine stays C++.
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
