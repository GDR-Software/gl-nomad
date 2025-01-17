#ifndef __G_ARCHIVE__
#define __G_ARCHIVE__

#pragma once

#include "../engine/n_cvar.h"
#include <EASTL/stack.h>

#define MAX_SAVE_FIELD_NAME 128
#define MAX_SAVE_SECTION_NAME 64
#define MAX_SAVE_SECTION_DEPTH 64

// this is inspired by a problem some people may have experienced with games that allow for multiple mods and saving that mod
// data to the actual save file. That's my experience with Bannerlord. If you define this, then each different section will
// also write an additional .prt file containing that individual data so that if the main save file is corrupted, someone
// can still use the existing uncorrupted .prt files to reconstruct the clean save data. This does, hovewever, use much more
// disk space especially depending on how much the game saves.
//
// so overall only really designed for PC users
#ifndef SAVEFILE_MOD_SAFETY
//    #define SAVEFILE_MOD_SAFETY
#endif

typedef struct {
	char name[MAX_NPATH];
	int32_t nVersionMajor;
	int32_t nVersionUpdate;
	int32_t nVersionPatch;
} modlist_t;

typedef struct {
	int32_t mapIndex;
	int32_t highestDif;
	int32_t saveDif;
	uint32_t playTimeHours;
	uint32_t playTimeMinutes;

	// mod info
	modlist_t *modList;
	uint64_t numMods;
} gamedata_t;

#pragma pack( push, 1 )
typedef struct {
	char *name;
	int32_t nameLength;
	int32_t type;
	int32_t dataSize;
	int32_t dataOffset;
	union {
		int8_t s8;
		int16_t s16;
		int32_t s32;
		int64_t s64;
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		float f;
		char *str;
		vec2_t v2;
		vec3_t v3;
		vec4_t v4;
	} data;
} ngdfield_t;

// version, 64 bits
typedef union version_s {
	struct {
		uint16_t m_nVersionMajor;
		uint16_t m_nVersionUpdate;
		uint32_t m_nVersionPatch;
	};
	uint64_t m_nVersion;

	bool operator==( const version_s& other ) const {
		return m_nVersion == other.m_nVersion;
	}
	bool operator!=( const version_s& other ) const {
		return m_nVersion != other.m_nVersion;
	}
} version_t;

typedef struct {
	int32_t ident;
	version_t version;
} ngdvalidation_t;
#pragma pack( pop )

typedef struct {
	ngdvalidation_t validation;
	int64_t numSections;

	gamedata_t gamedata;
} ngdheader_t;

typedef struct {
	char name[MAX_SAVE_FIELD_NAME];
	int32_t nameLength;
	int32_t numFields;
	uint32_t offset;
#ifdef SAVEFILE_MOD_SAFETY
	fileHandle_t hFile;
	const char *m_pModuleName;
#endif
} ngdsection_write_t;

template<typename Key, typename Value>
using ArchiveCache = eastl::unordered_map<Key, Value, eastl::hash<Key>, eastl::equal_to<Key>, CHunkAllocator<h_low>, true>;

typedef struct ngdsection_read_s {
	char name[MAX_SAVE_FIELD_NAME];
	int32_t nameLength;
	int32_t size;
	int32_t numFields;
	
   ngdfield_t **m_pFieldCache;
} ngdsection_read_t;

typedef struct {
	char name[MAX_NPATH];
	
	gamedata_t gd;
	
	int64_t m_nSections;
	uint64_t m_nMods;
	ngdsection_read_t *m_pSectionList;
} ngd_file_t;

class CGameArchive
{
public:
	CGameArchive( void );
	~CGameArchive() = default;

	void DeleteSlot( uint64_t nSlot );

	void BeginSaveSection( const char *moduleName, const char *name );
	void EndSaveSection( void );

	const char **GetSaveFiles( uint64_t *nFiles ) const;
	uint64_t NumUsedSaveSlots( void ) const;
	qboolean SlotIsUsed( uint64_t nSlot ) const;

	void SaveFloat( const char *name, float data );

	void SaveByte( const char *name, uint8_t data );
	void SaveUShort( const char *name, uint16_t data );
	void SaveUInt( const char *name, uint32_t data );
	void SaveULong( const char *name, uint64_t data );

	void SaveChar( const char *name, int8_t data );
	void SaveShort( const char *name, int16_t data );
	void SaveInt( const char *name, int32_t data );
	void SaveLong( const char *name, int64_t data );

	void SaveVec2( const char *name, const vec2_t data );
	void SaveVec3( const char *name, const vec3_t data );
	void SaveVec4( const char *name, const vec4_t data );

	void SaveCString( const char *name, const char *data );
	void SaveString( const char *name, const string_t *pData );
	void SaveArray( const char *pszName, const CScriptArray *pData );

	float LoadFloat( const char *name, nhandle_t hSection );

	uint8_t LoadByte( const char *name, nhandle_t hSection );
	uint16_t LoadUShort( const char *name, nhandle_t hSection );
	uint32_t LoadUInt( const char *name, nhandle_t hSection );
	uint64_t LoadULong( const char *name, nhandle_t hSection );

	int8_t LoadChar( const char *name, nhandle_t hSection );
	int16_t LoadShort( const char *name, nhandle_t hSection );
	int32_t LoadInt( const char *name, nhandle_t hSection );
	int64_t LoadLong( const char *name, nhandle_t hSection );

	void LoadVec2( const char *name, vec2_t data, nhandle_t hSection );
	void LoadVec3( const char *name, vec3_t data, nhandle_t hSection );
	void LoadVec4( const char *name, vec4_t data, nhandle_t hSection );

	void LoadCString( const char *name, char *pBuffer, int32_t maxLength, nhandle_t hSection );
	void LoadString( const char *name, string_t *pString, nhandle_t hSection );
	void LoadArray( const char *pszName, CScriptArray *pData, nhandle_t hSection );

	bool Load( uint64_t nSlot );
	bool Save( uint64_t nSlot );
	bool LoadPartial( uint64_t nSlot, gamedata_t *gd );

	nhandle_t GetSection( const char *name );

	void InitCache( void );

	friend void G_InitArchiveHandler( void );
	friend void G_ShutdownArchiveHandler( void );
private:
	void SaveArray( const char *func, const char *name, const void *pData, uint32_t nBytes );

	void AddField( const char *name, int32_t type, const void *data, uint32_t dataSize );
	bool ValidateHeader( const void *header ) const;
	qboolean LoadArchiveFile( const char *filename, uint64_t index );
	const ngdfield_t *FindField( const char *name, int32_t type, nhandle_t hSection ) const;

	fileHandle_t m_hFile;
	
	int64_t m_nSections;
	int64_t m_nSectionDepth;
	ngdsection_write_t *m_pSection;
	ngdsection_write_t m_szSectionStack[MAX_SAVE_SECTION_DEPTH];

	ngd_file_t **m_pArchiveCache;
	char **m_pArchiveFileList;
	uint64_t m_nCurrentArchive;
	uint64_t m_nUsedSaveSlots;
};

void G_InitArchiveHandler( void );
void G_ShutdownArchiveHandler( void );

extern CGameArchive *g_pArchiveHandler;

#endif