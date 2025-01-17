#include "scriptstdstring.h"
#ifndef __psp2__
	#include <locale.h> // setlocale()
#endif
#include "../../engine/n_threads.h"
#include "../aswrappedcall.h"

#include "../module_stringfactory.hpp"
#include "../module_public.h"

CModuleStringFactory *g_pStringFactory;

CModuleStringFactory *GetStringFactorySingleton( void )
{
	if ( !g_pStringFactory ) {
		CThreadMutex mutex;
		
		CThreadAutoLock<CThreadMutex> lock( mutex );
		static CModuleStringFactory stringFactory;
		g_pStringFactory = &stringFactory;
	}
	return g_pStringFactory;
}

// This macro is used to avoid warnings about unused variables.
// Usually where the variables are only used in debug mode.
#define UNUSED_VAR(x) (void)(x)

static void ConstructString( string_t *thisPointer )
{
	new ( thisPointer ) string_t();
}

static void CopyConstructString(const string_t& other, string_t *thisPointer)
{
	new ( thisPointer ) string_t( other );
//	thisPointer->clear();
//	thisPointer->append( other.cbegin(), other.cend() );
}

static void DestructString(string_t *thisPointer)
{
	thisPointer->~string_t();
}

static string_t& AddAssignStringToString(const string_t& str, string_t& dest)
{
	// We don't register the method directly because some compilers
	// and standard libraries inline the definition, resulting in the
	// linker being unable to find the declaration.
	// Example: CLang/LLVM with XCode 4.3 on OSX 10.7
	dest += str;
	return dest;
}

// bool string::isEmpty()
// bool string::empty() // if AS_USE_STLNAMES == 1
static bool StringIsEmpty(const string_t& str)
{
	// We don't register the method directly because some compilers
	// and standard libraries inline the definition, resulting in the
	// linker being unable to find the declaration
	// Example: CLang/LLVM with XCode 4.3 on OSX 10.7
	return str.empty();
}

static string_t& AssignUInt32ToString(asDWORD i, string_t& dest)
{
	dest.sprintf( "%u", i );
	return dest;
}

static string_t& AddAssignUInt32ToString(asDWORD i, string_t& dest)
{
	dest.append_sprintf( "%u", i );
	return dest;
}

static string_t AddStringUInt32(const string_t& str, asDWORD i)
{
	return str + va( "%u", i );
}

static string_t AddInt32String(asINT32 i, const string_t& str)
{
	return string_t( va( "%i", i ) ) + str;
}

static string_t& AssignInt32ToString(asINT32 i, string_t& dest)
{
	dest.sprintf( "%i", i );
	return dest;
}

static string_t& AddAssignInt32ToString(asINT32 i, string_t& dest)
{
	dest.append_sprintf( "%i", i );
	return dest;
}

static string_t AddStringInt32(const string_t& str, asINT32 i)
{
	return str + va( "%i", i );
}

static string_t AddUInt32String(asDWORD i, const string_t& str)
{
	return string_t( va( "%u", i ) ) + str;
}

static string_t& AssignDoubleToString(double f, string_t& dest)
{
	dest.sprintf( "%lf", f );
	return dest;
}

static string_t& AddAssignDoubleToString(double f, string_t& dest)
{
	dest.append_sprintf( "%lf", f );
	return dest;
}

static string_t& AssignFloatToString(float f, string_t& dest)
{
	dest.sprintf( "%f", f );
	return dest;
}

static string_t& AddAssignFloatToString(float f, string_t& dest)
{
	dest.append_sprintf( "%f", f );
	return dest;
}

static string_t& AssignBoolToString(bool b, string_t& dest)
{
	dest.sprintf( "%s", b ? "true" : "false" );
	return dest;
}

static string_t& AddAssignBoolToString(bool b, string_t& dest)
{
	dest.append_sprintf( "%s", b ? "true" : "false" );
	return dest;
}

static string_t AddStringDouble(const string_t& str, double f)
{
	return str + va( "%lf", f );
}

static string_t AddDoubleString(double f, const string_t& str)
{
	return string_t( va( "%lf", f ) ) + str;
}

static string_t AddStringFloat(const string_t& str, float f)
{
	return str + va( "%f", f );
}

static string_t AddFloatString(float f, const string_t& str)
{
	return string_t( va( "%f", f ) )  + str;
}

static string_t AddStringBool(const string_t& str, bool b)
{
	return str + va( "%s", b ? "true" : "false" );
}

static string_t AddBoolString(bool b, const string_t& str)
{
	return string_t( va( "%s", b ? "true" : "false" ) ) + str;
}

static char *StringCharAt( asDWORD i, string_t& str)
{
	if( i >= str.size() )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException( "Out of range" );

		return NULL;
	}

	return &str[i];
}

// AngelScript signature:
// int string::opCmp(const string_t& in) const
static asDWORD StringCmp(const string_t& a, const string_t& b)
{
	return strcmp( a.c_str(), b.c_str() );
}

// This function returns the index of the first position where the substring
// exists in the input string. If the substring doesn't exist in the input
// string_t -1 is returned.
//
// AngelScript signature:
// int string::findFirst(const string_t& in sub, uint start = 0) const
static asDWORD StringFindFirst(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.find(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// This function returns the index of the first position where the one of the bytes in substring
// exists in the input string. If the characters in the substring doesn't exist in the input
// string_t -1 is returned.
//
// AngelScript signature:
// int string::findFirstOf(const string_t& in sub, uint start = 0) const
static asDWORD StringFindFirstOf(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.find_first_of(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// This function returns the index of the last position where the one of the bytes in substring
// exists in the input string. If the characters in the substring doesn't exist in the input
// string_t -1 is returned.
//
// AngelScript signature:
// int string::findLastOf(const string_t& in sub, uint start = -1) const
static asDWORD StringFindLastOf(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.find_last_of(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// This function returns the index of the first position where a byte other than those in substring
// exists in the input string. If none is found -1 is returned.
//
// AngelScript signature:
// int string::findFirstNotOf(const string_t& in sub, uint start = 0) const
static asDWORD StringFindFirstNotOf(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.find_first_not_of(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// This function returns the index of the last position where a byte other than those in substring
// exists in the input string. If none is found -1 is returned.
//
// AngelScript signature:
// int string::findLastNotOf(const string_t& in sub, uint start = -1) const
static asDWORD StringFindLastNotOf(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.find_last_not_of(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// This function returns the index of the last position where the substring
// exists in the input string. If the substring doesn't exist in the input
// string_t -1 is returned.
//
// AngelScript signature:
// int string::findLast(const string_t& in sub, int start = -1) const
static asDWORD StringFindLast(const string_t& sub, asDWORD start, const string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (asQWORD)str.rfind(sub, (asQWORD)(start < 0 ? string_t::npos : start));
}

// AngelScript signature:
// void string::insert(uint pos, const string_t& in other)
static void StringInsert( asDWORD pos, const string_t& other, string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.insert(pos, other);
}

// AngelScript signature:
// void string::erase(uint pos, int count = -1)
static void StringErase( asDWORD pos, asDWORD count, string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.erase(pos, (asQWORD)(count < 0 ? string_t::npos : count));
}


// AngelScript signature:
// uint string::length() const
static asDWORD StringLength(const string_t& str)
{
	// We don't register the method directly because the return type changes between 32bit and 64bit platforms
	return str.length();
}


// AngelScript signature:
// void string::resize(uint l)
static void StringResize(asDWORD l, string_t& str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.resize(l);
}

// AngelScript signature:
// string_t formatInt(int64 val, const string_t& in options, uint width)
static string_t formatInt(asINT32 value, const string_t& format)
{
	return string_t().sprintf( format.c_str(), value );
}

// AngelScript signature:
// string_t formatUInt(uint64 val, const string_t& in options, uint width)
static string_t formatUInt(asDWORD value, const string_t& format)
{
	return string_t().sprintf( format.c_str(), value );
}

// AngelScript signature:
// string_t formatFloat(double val, const string_t& in options, uint width, uint precision)
static string_t formatFloat(double value, const string_t& format)
{
	return string_t().sprintf( format.c_str(), value );
}

// AngelScript signature:
// int32 parseInt(const string_t& in val, uint base = 10, uint &out byteCount = 0)
static asINT32 parseInt(const string_t& val, asUINT base, asUINT *byteCount)
{
	// Only accept base 10 and 16
	if( base != 10 && base != 16 )
	{
		if( byteCount ) *byteCount = 0;
		return 0;
	}

	const char *end = &val[0];

	// Determine the sign
	bool sign = false;
	if( *end == '-' )
	{
		sign = true;
		end++;
	}
	else if( *end == '+' )
		end++;

	asINT64 res = 0;
	if( base == 10 )
	{
		while( *end >= '0' && *end <= '9' )
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if( base == 16 )
	{
		while( (*end >= '0' && *end <= '9') ||
		       (*end >= 'a' && *end <= 'f') ||
		       (*end >= 'A' && *end <= 'F') )
		{
			res *= 16;
			if( *end >= '0' && *end <= '9' )
				res += *end++ - '0';
			else if( *end >= 'a' && *end <= 'f' )
				res += *end++ - 'a' + 10;
			else if( *end >= 'A' && *end <= 'F' )
				res += *end++ - 'A' + 10;
		}
	}

	if( byteCount )
		*byteCount = asUINT(asQWORD(end - val.c_str()));

	if( sign )
		res = -res;

	return res;
}

// AngelScript signature:
// uint64 parseUInt(const string_t& in val, uint base = 10, uint &out byteCount = 0)
static asDWORD parseUInt(const string_t& val, asUINT base, asUINT *byteCount)
{
	// Only accept base 10 and 16
	if (base != 10 && base != 16)
	{
		if (byteCount) *byteCount = 0;
		return 0;
	}

	const char *end = &val[0];

	asDWORD res = 0;
	if (base == 10)
	{
		while (*end >= '0' && *end <= '9')
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if (base == 16)
	{
		while ((*end >= '0' && *end <= '9') ||
			(*end >= 'a' && *end <= 'f') ||
			(*end >= 'A' && *end <= 'F'))
		{
			res *= 16;
			if (*end >= '0' && *end <= '9')
				res += *end++ - '0';
			else if (*end >= 'a' && *end <= 'f')
				res += *end++ - 'a' + 10;
			else if (*end >= 'A' && *end <= 'F')
				res += *end++ - 'A' + 10;
		}
	}

	if (byteCount)
		*byteCount = asUINT(asQWORD(end - val.c_str()));

	return res;
}

// AngelScript signature:
// double parseFloat(const string_t& in val, uint &out byteCount = 0)
double parseFloat(const string_t& val, asUINT *byteCount)
{
	char *end;

	// WinCE doesn't have setlocale. Some quick testing on my current platform
	// still manages to parse the numbers such as "3.14" even if the decimal for the
	// locale is ",".
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
	// Set the locale to C so that we are guaranteed to parse the float value correctly
	char *tmp = setlocale(LC_NUMERIC, 0);
	string_t orig = tmp ? tmp : "C";
	setlocale(LC_NUMERIC, "C");
#endif

	double res = strtod(val.c_str(), &end);

#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
	// Restore the locale
	setlocale(LC_NUMERIC, orig.c_str());
#endif

	if( byteCount )
		*byteCount = asUINT(asQWORD(end - val.c_str()));

	return res;
}

// This function returns a string_t containing the substring of the input string
// determined by the starting index and count of characters.
//
// AngelScript signature:
// string_t string::substr(uint start = 0, int count = -1) const
static string_t StringSubString(asUINT start, int count, const string_t& str)
{
	// Check for out-of-bounds
	string_t ret;
	if( start < str.length() && count != 0 )
		ret = str.substr(start, (asQWORD)(count < 0 ? string_t::npos : count));

	return ret;
}

// String equality comparison.
// Returns true iff lhs is equal to rhs.
//
// For some reason gcc 4.7 has difficulties resolving the
// asFUNCTIONPR(operator==, (const string_t& , const string_t& )
// makro, so this wrapper was introduced as work around.
static bool StringEquals(const string_t& lhs, const string_t& rhs)
{
	return strcmp( lhs.c_str(), rhs.c_str() ) == 0;
}

static string_t StringAppend( const string_t& value, const string_t& add )
{
	return value + add;
}

void RegisterStdString_Native(asIScriptEngine *engine)
{
	// Register the string_t type
#if AS_CAN_USE_CPP11
	// With C++11 it is possible to use asGetTypeTraits to automatically determine the correct flags to use
	CheckASCall( engine->RegisterObjectType("string", sizeof(string_t), asOBJ_VALUE | asGetTypeTraits<string_t>()) );
#else
	CheckASCall( engine->RegisterObjectType("string", sizeof(string_t), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK) );
#endif

	CheckASCall( engine->RegisterStringFactory("string", GetStringFactorySingleton()) );

	// Register the object operator overloads
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f(const string& in)",    asFUNCTION(CopyConstructString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructString),  asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(const string& in)", asMETHODPR(string_t, operator=, (const string_t&), string_t&), asCALL_THISCALL) );
	// Need to use a wrapper on Mac OS X 10.7/XCode 4.3 and CLang/LLVM, otherwise the linker fails
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(const string& in)", asFUNCTION(AddAssignStringToString), asCALL_CDECL_OBJLAST) );
//	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(const string& in)", asMETHODPR(string, operator+=, (const string&), string&), asCALL_THISCALL) );

	// Need to use a wrapper for operator== otherwise gcc 4.7 fails to compile
	CheckASCall( engine->RegisterObjectMethod("string", "bool opEquals(const string& in) const", asFUNCTIONPR(StringEquals, (const string_t& , const string_t& ), bool), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int opCmp(const string& in) const", asFUNCTION(StringCmp), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(const string& in) const", asFUNCTION(StringAppend), asCALL_CDECL_OBJFIRST) );

	// The string_t length can be accessed through methods or through virtual property
	// TODO: Register as size() for consistency with other types
	CheckASCall( engine->RegisterObjectMethod("string", "uint Length() const", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "uint Size() const", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "void Resize( uint )", asFUNCTION(StringResize), asCALL_CDECL_OBJLAST) );
	// Need to use a wrapper on Mac OS X 10.7/XCode 4.3 and CLang/LLVM, otherwise the linker fails
//	CheckASCall( engine->RegisterObjectMethod("string", "bool isEmpty() const", asMETHOD(string, empty), asCALL_THISCALL) );
	CheckASCall( engine->RegisterObjectMethod("string", "bool IsEmpty() const", asFUNCTION(StringIsEmpty), asCALL_CDECL_OBJLAST) );

	// Register the index operator, both as a mutator and as an inspector
	// Note that we don't register the operator[] directly, as it doesn't do bounds checking
	CheckASCall( engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST) );

	// Automatic conversion from values
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddStringDouble), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDoubleString), asCALL_CDECL_OBJLAST) );

	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(AssignFloatToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(AddAssignFloatToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(AddStringFloat), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(AddFloatString), asCALL_CDECL_OBJLAST) );

	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(int32)", asFUNCTION(AssignInt32ToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(int32)", asFUNCTION(AddAssignInt32ToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(int32) const", asFUNCTION(AddStringInt32), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(int32) const", asFUNCTION(AddInt32String), asCALL_CDECL_OBJLAST) );

	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(uint32)", asFUNCTION(AssignUInt32ToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(uint32)", asFUNCTION(AddAssignUInt32ToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(uint32) const", asFUNCTION(AddStringUInt32), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(uint32) const", asFUNCTION(AddUInt32String), asCALL_CDECL_OBJLAST) );

	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBoolToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBoolToString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddStringBool), asCALL_CDECL_OBJFIRST) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBoolString), asCALL_CDECL_OBJLAST) );

	// Utilities
	CheckASCall( engine->RegisterObjectMethod("string", "string SubString(uint start = 0, int count = -1) const", asFUNCTION(StringSubString), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirst(const string& in, uint start = 0) const", asFUNCTION(StringFindFirst), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirstOf(const string& in, uint start = 0) const", asFUNCTION(StringFindFirstOf), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirstNotOf(const string& in, uint start = 0) const", asFUNCTION(StringFindFirstNotOf), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLast(const string& in, int start = -1) const", asFUNCTION(StringFindLast), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLastOf(const string& in, int start = -1) const", asFUNCTION(StringFindLastOf), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLastNotOf(const string& in, int start = -1) const", asFUNCTION(StringFindLastNotOf), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "void Insert(uint pos, const string& in other)", asFUNCTION(StringInsert), asCALL_CDECL_OBJLAST) );
	CheckASCall( engine->RegisterObjectMethod("string", "void Erase(uint pos, int count = -1)", asFUNCTION(StringErase), asCALL_CDECL_OBJLAST) );

	CheckASCall( engine->RegisterGlobalFunction("string formatInt(int32 val, const string& in options = \"%i\", uint width = 0)", asFUNCTION(formatInt), asCALL_CDECL) );
	CheckASCall( engine->RegisterGlobalFunction("string formatUInt(uint32 val, const string& in options = \"%u\", uint width = 0)", asFUNCTION(formatUInt), asCALL_CDECL) );
	CheckASCall( engine->RegisterGlobalFunction("string formatFloat(double val, const string& in options = \"%f\", uint width = 0, uint precision = 0)", asFUNCTION(formatFloat), asCALL_CDECL) );
	CheckASCall( engine->RegisterGlobalFunction("int32 parseInt(const string& in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseInt), asCALL_CDECL) );
	CheckASCall( engine->RegisterGlobalFunction("uint32 parseUInt(const string& in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseUInt), asCALL_CDECL) );
	CheckASCall( engine->RegisterGlobalFunction("double parseFloat(const string& in, uint &out byteCount = 0)", asFUNCTION(parseFloat), asCALL_CDECL) );

	// TODO: Implement the following
	// findAndReplace - replaces a text found in the string
	// replaceRange - replaces a range of bytes in the string
	// multiply/times/opMul/opMul_r - takes the string_t and multiplies it n times, e.g. "-".multiply(5) returns "-----"
}

static void ConstructStringGeneric( asIScriptGeneric *gen ) {
	new ( gen->GetObjectData() ) string_t();
}

static void CopyConstructStringGeneric(asIScriptGeneric *gen)
{
	const string_t *a = (string_t *)gen->GetArgObject( 0 );
	string_t *obj = (string_t *)gen->GetObjectData();

	new ( obj ) string_t();
	obj->append( a->cbegin(), a->cend() );
//	new (gen->GetObjectData()) string_t(*a);
}

static void DestructStringGeneric(asIScriptGeneric *gen)
{
	string_t *ptr = static_cast<string_t *>(gen->GetObjectData());
	ptr->~string_t();
}

static void AssignStringGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetArgObject(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	*self = *a;
	gen->SetReturnAddress(self);
}

static void AddAssignStringGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetArgObject(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	*self += *a;
	gen->SetReturnAddress(self);
}

static void StringEqualsGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	string_t *b = static_cast<string_t *>(gen->GetArgAddress(0));
	*(bool*)gen->GetAddressOfReturnLocation() = (*a == *b);
}

static void StringCmpGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	string_t *b = static_cast<string_t *>(gen->GetArgAddress(0));

	int cmp = 0;
	if( *a < *b ) cmp = -1;
	else if( *a > *b ) cmp = 1;

	*(int*)gen->GetAddressOfReturnLocation() = cmp;
}

static void StringAddGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	string_t *b = static_cast<string_t *>(gen->GetArgAddress(0));
	string_t ret_val = *a + *b;
	gen->SetReturnObject(&ret_val);
}

static void StringLengthGeneric(asIScriptGeneric *gen)
{
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	*static_cast<asUINT *>(gen->GetAddressOfReturnLocation()) = (asUINT)self->length();
}

static void StringIsEmptyGeneric(asIScriptGeneric *gen)
{
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<bool *>(gen->GetAddressOfReturnLocation()) = StringIsEmpty(*self);
}

static void StringResizeGeneric(asIScriptGeneric *gen)
{
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->resize(*static_cast<asUINT *>(gen->GetAddressOfArg(0)));
}

static void StringReserveGeneric( asIScriptGeneric *gen )
{
	string_t *self = (string_t *)gen->GetObjectData();
	self->reserve( *(asUINT *)gen->GetAddressOfArg( 0 ) );
}

static void StringInsert_Generic(asIScriptGeneric *gen)
{
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	asUINT pos = gen->GetArgDWord(0);
	string_t *other = reinterpret_cast<string_t *>(gen->GetArgAddress(1));
	StringInsert(pos, *other, *self);
}

static void StringErase_Generic(asIScriptGeneric *gen)
{
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	asUINT pos = gen->GetArgDWord(0);
	int count = int(gen->GetArgDWord(1));
	StringErase(pos, count, *self);
}

static void StringFindFirst_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirst(*find, start, *self);
}

static void StringFindLast_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLast(*find, start, *self);
}

static void StringFindFirstOf_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirstOf(*find, start, *self);
}

static void StringFindLastOf_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLastOf(*find, start, *self);
}

static void StringFindFirstNotOf_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirstNotOf(*find, start, *self);
}

static void StringFindLastNotOf_Generic(asIScriptGeneric *gen)
{
	string_t *find = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string_t *self = reinterpret_cast<string_t *>(gen->GetObjectData());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLastNotOf(*find, start, *self);
}

static void formatInt_Generic(asIScriptGeneric *gen)
{
	asINT32 val = gen->GetArgDWord(0);
	string_t *fmt = reinterpret_cast<string_t*>(gen->GetArgAddress(1));
	new(gen->GetAddressOfReturnLocation()) string_t(formatInt(val, *fmt));
}

static void formatUInt_Generic(asIScriptGeneric *gen)
{
	asDWORD val = gen->GetArgDWord(0);
	string_t *fmt = reinterpret_cast<string_t*>(gen->GetArgAddress(1));
	new(gen->GetAddressOfReturnLocation()) string_t(formatUInt(val, *fmt));
}

static void formatFloat_Generic(asIScriptGeneric *gen)
{
	double val = gen->GetArgDouble(0);
	string_t *fmt = reinterpret_cast<string_t*>(gen->GetArgAddress(1));
	new(gen->GetAddressOfReturnLocation()) string_t(formatFloat(val, *fmt));
}

static void parseInt_Generic(asIScriptGeneric *gen)
{
	string_t *str = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT base = gen->GetArgDWord(1);
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(2));
	gen->SetReturnQWord(parseInt(*str,base,byteCount));
}

static void parseUInt_Generic(asIScriptGeneric *gen)
{
	string_t *str = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT base = gen->GetArgDWord(1);
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(2));
	gen->SetReturnQWord(parseUInt(*str, base, byteCount));
}

static void parseFloat_Generic(asIScriptGeneric *gen)
{
	string_t *str = reinterpret_cast<string_t*>(gen->GetArgAddress(0));
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(1));
	gen->SetReturnDouble(parseFloat(*str,byteCount));
}

static void StringCharAtGeneric(asIScriptGeneric *gen)
{
	unsigned int index = gen->GetArgDWord(0);
	string_t *self = static_cast<string_t *>(gen->GetObjectData());

	if (index >= self->size())
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		gen->SetReturnAddress(0);
	}
	else
	{
		gen->SetReturnAddress(&(self->operator [](index)));
	}
}

static void AssignInt2StringGeneric(asIScriptGeneric *gen)
{
	asINT32 *a = static_cast<asINT32*>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t*>(gen->GetObjectData());
	self->append_sprintf( "%lu", *a );
	gen->SetReturnAddress(self);
}

static void AssignUInt2StringGeneric(asIScriptGeneric *gen)
{
	asDWORD *a = static_cast<asDWORD*>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t*>(gen->GetObjectData());
	self->append_sprintf( "%lu", *a );
	gen->SetReturnAddress(self);
}

static void AssignDouble2StringGeneric(asIScriptGeneric *gen)
{
	double *a = static_cast<double*>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t*>(gen->GetObjectData());
	self->append_sprintf( "%0.02lf", *a );
	gen->SetReturnAddress(self);
}

static void AssignFloat2StringGeneric(asIScriptGeneric *gen)
{
	float *a = static_cast<float*>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t*>(gen->GetObjectData());
	self->append_sprintf( "%0.02f", *a );
	gen->SetReturnAddress(self);
}

static void AssignBool2StringGeneric(asIScriptGeneric *gen)
{
	bool *a = static_cast<bool*>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t*>(gen->GetObjectData());
	self->append_sprintf( "%s", *a ? "true" : "false" );
	gen->SetReturnAddress(self);
}

static void AddAssignDouble2StringGeneric(asIScriptGeneric *gen)
{
	double *a = static_cast<double *>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->append_sprintf( "%0.02lf", *a );
	gen->SetReturnAddress(self);
}

static void AddAssignFloat2StringGeneric(asIScriptGeneric *gen)
{
	float *a = static_cast<float *>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->append_sprintf( "%0.02f", *a );
	gen->SetReturnAddress(self);
}

static void AddAssignInt2StringGeneric(asIScriptGeneric *gen)
{
	asINT32 *a = static_cast<asINT32 *>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->append_sprintf( "%li", *a );
	gen->SetReturnAddress(self);
}

static void AddAssignUInt2StringGeneric(asIScriptGeneric *gen)
{
	asDWORD *a = static_cast<asDWORD *>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->append_sprintf("%lu", *a );
	gen->SetReturnAddress(self);
}

static void AddAssignBool2StringGeneric(asIScriptGeneric *gen)
{
	bool *a = static_cast<bool *>(gen->GetAddressOfArg(0));
	string_t *self = static_cast<string_t *>(gen->GetObjectData());
	self->append_sprintf( "%s", *a ? "true" : "false" );
	gen->SetReturnAddress(self);
}

static void AddString2DoubleGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	double *b = static_cast<double *>(gen->GetAddressOfArg(0));
	string_t ret_val;
	ret_val.sprintf( "%s%0.02lf", a->c_str(), *b );
	gen->SetReturnObject(&ret_val);
}

static void AddString2FloatGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	float *b = static_cast<float *>(gen->GetAddressOfArg(0));
	string_t ret_val;
	ret_val.sprintf( "%s%0.02f", a->c_str(), *b );
	gen->SetReturnObject(&ret_val);
}

static void AddString2IntGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	asINT32 *b = static_cast<asINT32 *>(gen->GetAddressOfArg(0));
	string_t ret_val;
	ret_val.sprintf( "%s%li", a->c_str(), *b );
	gen->SetReturnObject(&ret_val);
}

static void AddString2UIntGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	asDWORD *b = static_cast<asDWORD *>(gen->GetAddressOfArg(0));
	string_t ret_val;
	ret_val.sprintf( "%s%lu", a->c_str(), *b );
	gen->SetReturnObject(&ret_val);
}

static void AddString2BoolGeneric(asIScriptGeneric *gen)
{
	string_t *a = static_cast<string_t *>(gen->GetObjectData());
	bool *b = static_cast<bool *>(gen->GetAddressOfArg(0));
	string_t ret_val;
	ret_val.sprintf( "%s%s", a->c_str(), ( *b ? "true" : "false" ) );
	gen->SetReturnObject(&ret_val);
}

static void AddDouble2StringGeneric(asIScriptGeneric *gen)
{
	double *a = static_cast<double *>(gen->GetAddressOfArg(0));
	string_t *b = static_cast<string_t *>(gen->GetObjectData());
	string_t ret_val;
	ret_val.sprintf( "%0.02lf%s", *a, b->c_str() );
	gen->SetReturnObject(&ret_val);
}

static void AddFloat2StringGeneric(asIScriptGeneric *gen)
{
	float*a = static_cast<float *>(gen->GetAddressOfArg(0));
	string_t *b = static_cast<string_t *>(gen->GetObjectData());

	string_t ret_val = va( "%0.02f%s", *a, b->c_str() );
	gen->SetReturnObject(&ret_val);
}

static void AddInt2StringGeneric(asIScriptGeneric *gen)
{
	asINT32 *a = static_cast<asINT32 *>(gen->GetAddressOfArg(0));
	string_t *b = static_cast<string_t *>(gen->GetObjectData());
	string_t ret_val = va( "%i%s", *a, b->c_str() );
	gen->SetReturnObject(&ret_val);
}

static void AddUInt2StringGeneric(asIScriptGeneric *gen)
{
	asDWORD *a = static_cast<asDWORD *>(gen->GetAddressOfArg(0));
	string_t *b = static_cast<string_t *>(gen->GetObjectData());
	string_t ret_val = va( "%u%s", *a, b->c_str() );;
	gen->SetReturnObject(&ret_val);
}

static void AddBool2StringGeneric(asIScriptGeneric *gen)
{
	bool*a = static_cast<bool *>(gen->GetAddressOfArg(0));
	string_t *b = static_cast<string_t *>(gen->GetObjectData());
	string_t ret_val = va( "%s%s", ( *a ? "true" : "false" ), b->c_str() );
	gen->SetReturnObject(&ret_val);
}

static void StringSubString_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	string_t *str   = (string_t*)gen->GetObjectData();
	asUINT  start = *(int*)gen->GetAddressOfArg(0);
	int     count = *(int*)gen->GetAddressOfArg(1);

	// Return the substring
	new(gen->GetAddressOfReturnLocation()) string_t(StringSubString(start, count, *str));
}

static void StringIteratorBegin_Generic( asIScriptGeneric *gen )
{
	string_t *str = (string_t *)gen->GetObjectData();
	gen->SetReturnAddress( str->begin() );
}

static void StringIteratorEnd_Generic( asIScriptGeneric *gen )
{
	string_t *str = (string_t *)gen->GetObjectData();
	gen->SetReturnAddress( str->end() );
}

static void StringIteratorCBegin_Generic( asIScriptGeneric *gen )
{
	const string_t *str = (const string_t *)gen->GetObjectData();
	gen->SetReturnAddress( const_cast<char *>( str->cbegin() ) );
}

static void StringIteratorCEnd_Generic( asIScriptGeneric *gen )
{
	const string_t *str = (const string_t *)gen->GetObjectData();
	gen->SetReturnAddress( const_cast<char *>( str->cend() ) );
}

static void StringFind_Generic( asIScriptGeneric *gen )
{
	const string_t *str = (const string_t *)gen->GetObjectData();
	gen->SetReturnQWord( str->find( (char)gen->GetArgByte( 0 ), 0 ) );
}

static void StringFindStr_Generic( asIScriptGeneric *gen )
{
	const string_t *str = (const string_t *)gen->GetObjectData();
	const string_t *other = (const string_t *)gen->GetArgObject( 0 );
	gen->SetReturnQWord( str->find( *other, 0 ) );
}

static const size_t StringNPos = size_t( -1 );

void RegisterStdString_Generic(asIScriptEngine *engine)
{
	// Register the string_t type
	CheckASCall( engine->RegisterObjectType("string", sizeof(string_t), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ) );

	CheckASCall( engine->RegisterStringFactory("string", GetStringFactorySingleton()) );

	CheckASCall( engine->RegisterGlobalProperty( "const uint64 StringNPos", const_cast<size_t *>( &StringNPos ) ) );

	// Register the object operator overloads
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructStringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f(const string &in)",    asFUNCTION(CopyConstructStringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructStringGeneric),  asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(AssignStringGeneric),    asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(AddAssignStringGeneric), asCALL_GENERIC ) );

	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDouble2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDouble2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddString2DoubleGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDouble2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTION(StringEqualsGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(StringCmpGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTION(StringAddGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string& Concat(bool)", asFUNCTION(AddAssignBool2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(AssignFloat2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(AddAssignFloat2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(AddString2FloatGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(AddFloat2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(int64)", asFUNCTION(AssignInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(int64)", asFUNCTION(AddAssignInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(AddString2IntGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(AddInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(AssignUInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(AddAssignUInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(AddString2UIntGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(AddUInt2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBool2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBool2StringGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddString2BoolGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBool2StringGeneric), asCALL_GENERIC ) );

	CheckASCall( engine->RegisterObjectMethod( "string", "uint64 Find( int8 ) const", asFUNCTION( StringFind_Generic ), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod( "string", "uint64 Find( const string& in ) const", asFUNCTION( StringFindStr_Generic ), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod( "string", "uint Length() const", asFUNCTION(StringLengthGeneric), asCALL_GENERIC ) );

	CheckASCall( engine->RegisterObjectMethod("string", "bool Empty() const", asFUNCTION(StringIsEmptyGeneric), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "string SubString(uint start = 0, int count = -1) const", asFUNCTION(StringSubString_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirst(const string &in, uint start = 0) const", asFUNCTION(StringFindFirst_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirstOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstOf_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindFirstNotOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstNotOf_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLast(const string &in, int start = -1) const", asFUNCTION(StringFindLast_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLastOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastOf_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "int FindLastNotOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastNotOf_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "void Insert(uint pos, const string &in other)", asFUNCTION(StringInsert_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterObjectMethod("string", "void Erase(uint pos, int count = -1)", asFUNCTION(StringErase_Generic), asCALL_GENERIC ) );

	CheckASCall( engine->RegisterGlobalFunction("string formatInt(int32 val, const string& in formatting = \"%i\")", asFUNCTION(formatInt_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterGlobalFunction("string formatUInt(uint32 val, const string& in formatting = \"%u\")", asFUNCTION(formatUInt_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterGlobalFunction("string formatFloat(double val, const string& in formatting = \"%0.02f\")", asFUNCTION(formatFloat_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterGlobalFunction("int64 parseInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseInt_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterGlobalFunction("uint64 parseUInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseUInt_Generic), asCALL_GENERIC ) );
	CheckASCall( engine->RegisterGlobalFunction("double parseFloat(const string &in, uint &out byteCount = 0)", asFUNCTION(parseFloat_Generic), asCALL_GENERIC ) );
}

void RegisterStdString(asIScriptEngine *engine)
{
	RegisterStdString_Native(engine);
}
