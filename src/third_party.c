// Single-file library implementations (compiled once, as C).
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "miniaudio.h"
#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
