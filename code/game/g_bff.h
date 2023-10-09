#ifndef _G_BFF_
#define _G_BFF_

#ifdef GDR_PRAGMA_ONCE_SUPPORTED
	#pragma once
#endif

#ifdef __cplusplus

#define BFF_VERSION_MAJOR 0
#define BFF_VERSION_MINOR 1
#define BFF_VERSION ((BFF_VERSION_MAJOR<<8)+BFF_VERSION_MINOR)

#define HEADER_MAGIC 0x5f3759df
#define BFF_IDENT (('B'<<24)+('F'<<16)+('F'<<8)+'I')
#define BFF_STR_SIZE (int)80
#define DEFAULT_BFF_GAMENAME "bffNoName"

enum : uint64_t
{
	LEVEL_CHUNK,
	SOUND_CHUNK,
	SCRIPT_CHUNK,
	TEXTURE_CHUNK
};

#define BFF_Error N_Error

const __inline int64_t bffPaidID[70] = {
	0x0000, 0x0000, 0x0412, 0x0000, 0x0000, 0x0000, 0x0527,
	0x421f, 0x0400, 0x0000, 0x0383, 0x0000, 0x0000, 0x0000,
	0x0000, 0x5666, 0x6553, 0x0000, 0x0000, 0xad82, 0x8270,
	0x2480, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x053d, 0x0000, 0x0000, 0x7813, 0x0000,
	0x02aa, 0x0100, 0x0000, 0x4941, 0x0000, 0x3334, 0xff42,
	0x0000, 0x0546, 0xa547, 0x1321, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0xa235, 0xfa31, 0x215a, 0x0000
};

constexpr uint64_t MAGIC_XOR = 0x4ff3ade3;

constexpr uint64_t MAX_BFF_PATH = 256;
constexpr uint64_t MAX_BFF_CHUNKNAME = 72;
constexpr uint64_t MAX_TEXTURE_CHUNKS = 128;
constexpr uint64_t MAX_LEVEL_CHUNKS = 128;
constexpr uint64_t MAX_SOUND_CHUNKS = 128;
constexpr uint64_t MAX_SCRIPT_CHUNKS = 64;
constexpr uint64_t MAX_MAP_SPAWNS = 1024;
constexpr uint64_t MAX_MAP_LIGHTS = 1024;

enum : int64_t {
	COMPRESSION_NONE,
	COMPRESSION_BZIP2,
	COMPRESSION_ZLIB
};

enum : int64_t
{
	SFT_OGG,
	SFT_WAV,
	SFT_OPUS
};

enum : int64_t
{
	TEX_JPG,
	TEX_BMP,
	TEX_TGA,
	TEX_PNG,
};

#else

#define MAGIC_XOR 0x4ff3ade3

#define MAX_BFF_PATH 256
#define MAX_BFF_CHUNKNAME 72
#define MAX_TEXTURE_CHUNKS 128
#define MAX_LEVEL_CHUNKS 128
#define MAX_SOUND_CHUNKS 128
#define MAX_SCRIPT_CHUNKS 64
#define MAX_MAP_SPAWNS 1024
#define MAX_MAP_LIGHTS 1024

#endif

typedef struct
{
	char name[MAX_BFF_CHUNKNAME];
	char *fileBuffer;
	
	int64_t fileSize;
	int64_t fileType;
} bffsound_t;

typedef struct
{
	// bff stuff
	char name[MAX_BFF_CHUNKNAME];
	int64_t levelNumber;

	char *levelBuffer;
	int64_t levelBufferLen;

	char *tmjBuffer;
	char **tsjBuffers;

	int64_t mapBufferLen;
	int64_t numTilesets;
} bfflevel_t;

// scripted encounters (boss fights, story mode, etc.)
typedef struct
{
	char name[MAX_BFF_CHUNKNAME];
	
	int64_t codelen;
	uint8_t* bytecode; // q3vm raw bytecode
} bffscript_t;

typedef struct
{
	char name[MAX_BFF_CHUNKNAME];

	int64_t fileSize;
	int64_t fileType;
	unsigned char *fileBuffer;
} bfftexture_t;


typedef struct bffinfo_s
{
	bfftexture_t textures[MAX_TEXTURE_CHUNKS];
	bfflevel_t levels[MAX_LEVEL_CHUNKS];
	bffscript_t scripts[MAX_SCRIPT_CHUNKS];
	bffsound_t sounds[MAX_SOUND_CHUNKS];

	char bffPathname[MAX_BFF_PATH];
	char bffGamename[256];
	
	int64_t compression;
	int64_t numTextures;
	int64_t numLevels;
	int64_t numSounds;
	int64_t numScripts;
} bffinfo_t;

typedef struct
{
	char *chunkName;
	int64_t chunkNameLen;
	int64_t chunkSize;
	char *chunkBuffer;
} bff_chunk_t;

typedef struct
{
	int64_t ident;
	int64_t magic;
	int64_t numChunks;
	int64_t compression;
	int16_t version;
} bffheader_t;

typedef struct
{
	char *bffGamename;
	bffheader_t header;
	
	int64_t numChunks;
	bff_chunk_t* chunkList;
} bff_t;

void B_GetChunk(const char *chunkname);
void BFF_CloseArchive(bff_t* archive);
bff_t* BFF_OpenArchive(const char *filepath);
bffinfo_t* BFF_FetchInfo(void);
void BFF_FreeInfo(bffinfo_t* info);

bffscript_t* BFF_FetchScript(const char *name);
bfflevel_t* BFF_FetchLevel(const char *name);
bfftexture_t* BFF_FetchTexture(const char *name);

void G_LoadBFF(const char *bffname);

#endif