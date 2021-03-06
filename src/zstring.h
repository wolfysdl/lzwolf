/*
** zstring.h
**
**---------------------------------------------------------------------------
** Copyright 2005-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifndef ZSTRING_H
#define ZSTRING_H

#include <string>
#include <memory>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "tarray.h"
#include "name.h"

// Android doesn't apply here since printf is redefined to be LOGI
#if defined(__GNUC__) && !defined(__ANDROID__)
#define PRINTFISH(x) __attribute__((format(printf, 2, x)))
#else
#define PRINTFISH(x)
#endif

#if defined(__GNUC__) && !defined(__ANDROID__)
#define GCCPRINTF(stri,firstargi)		__attribute__((format(printf,stri,firstargi)))
#define GCCFORMAT(stri)					__attribute__((format(printf,stri,0)))
#define GCCNOWARN						__attribute__((unused))
#else
#define GCCPRINTF(a,b)
#define GCCFORMAT(a)
#define GCCNOWARN
#endif

extern "C" int mysnprintf(char *buffer, size_t count, const char *format, ...) GCCPRINTF(3,4);
extern "C" int myvsnprintf(char *buffer, size_t count, const char *format, va_list argptr) GCCFORMAT(3);

enum ELumpNum
{
};

class FString
{
public:
	FString () : CharsPtr(std::make_shared<std::string>()), LockCount(0) {}

	// Copy constructors
	FString (const FString &other);
	FString (const char *copyStr);
	FString (const char *copyStr, size_t copyLen);
	FString (char oneChar);

	// Concatenation constructors
	FString (const FString &head, const FString &tail);
	FString (const FString &head, const char *tail);
	FString (const FString &head, char tail);
	FString (const char *head, const FString &tail);
	FString (const char *head, const char *tail);
	FString (char head, const FString &tail);

	// Other constructors
	FString (ELumpNum);	// Create from a lump

	// Discard string's contents, create a new buffer, and lock it.
	char *LockNewBuffer(size_t len);

	char *LockBuffer();		// Obtain write access to the character buffer
	void UnlockBuffer();	// Allow shared access to the character buffer

	operator const char *() const
	{
		return CharsPtr->c_str();
	}

	const char *GetChars() const
	{
		return CharsPtr->c_str();
	}

	const char &operator[] (int index) const { return (*CharsPtr)[index]; }
#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER)
	// Compiling 32-bit Windows source with MSVC: size_t is typedefed to an
	// unsigned int with the 64-bit portability warning attribute, so the
	// prototype cannot substitute unsigned int for size_t, or you get
	// spurious warnings.
	const char &operator[] (size_t index) const { return (*CharsPtr)[index]; }
#else
	const char &operator[] (unsigned int index) const { return (*CharsPtr)[index]; }
#endif
	const char &operator[] (unsigned long index) const { return (*CharsPtr)[index]; }
	const char &operator[] (unsigned long long index) const { return (*CharsPtr)[index]; }

	FString &operator = (const FString &other);
	FString &operator = (const char *copyStr);

	FString operator + (const FString &tail) const;
	FString operator + (const char *tail) const;
	FString operator + (char tail) const;
	friend FString operator + (const char *head, const FString &tail);
	friend FString operator + (char head, const FString &tail);

	FString &operator += (const FString &tail);
	FString &operator += (const char *tail);
	FString &operator += (char tail);
	FString &operator += (const FName &name) { return *this += name.GetChars(); }
	FString &AppendCStrPart (const char *tail, size_t tailLen);

	FString &operator << (const FString &tail) { return *this += tail; }
	FString &operator << (const char *tail) { return *this += tail; }
	FString &operator << (char tail) { return *this += tail; }
	FString &operator << (const FName &name) { return *this += name.GetChars(); }

	FString Left (size_t numChars) const;
	FString Right (size_t numChars) const;
	FString Mid (size_t pos, size_t numChars = ~(size_t)0) const;

	long IndexOf (const FString &substr, long startIndex=0) const;
	long IndexOf (const char *substr, long startIndex=0) const;
	long IndexOf (char subchar, long startIndex=0) const;

	long IndexOfAny (const FString &charset, long startIndex=0) const;
	long IndexOfAny (const char *charset, long startIndex=0) const;

	long LastIndexOf (const FString &substr) const;
	long LastIndexOf (const char *substr) const;
	long LastIndexOf (char subchar) const;
	long LastIndexOf (const FString &substr, long endIndex) const;
	long LastIndexOf (const char *substr, long endIndex) const;
	long LastIndexOf (char subchar, long endIndex) const;
	long LastIndexOf (const char *substr, long endIndex, size_t substrlen) const;

	long LastIndexOfAny (const FString &charset) const;
	long LastIndexOfAny (const char *charset) const;
	long LastIndexOfAny (const FString &charset, long endIndex) const;
	long LastIndexOfAny (const char *charset, long endIndex) const;

	void ToUpper ();
	void ToLower ();
	void SwapCase ();

	void Insert (size_t index, const FString &instr);
	void Insert (size_t index, const char *instr);
	void Insert (size_t index, const char *instr, size_t instrlen);

	void ReplaceChars (char oldchar, char newchar);
	void ReplaceChars (const char *oldcharset, char newchar);

	void StripChars (char killchar);
	void StripChars (const char *killchars);

	void Format (const char *fmt, ...) PRINTFISH(3);
	void AppendFormat (const char *fmt, ...) PRINTFISH(3);
	void VFormat (const char *fmt, va_list arglist) PRINTFISH(0);
	void VAppendFormat (const char *fmt, va_list arglist) PRINTFISH(0);

	bool IsInt () const;
	bool IsFloat () const;
	long ToLong (int base=0) const;
	unsigned long ToULong (int base=0) const;
	double ToDouble () const;

	size_t Len() const { return CharsPtr->length(); }
	bool IsEmpty() const { return Len() == 0; }
	bool IsNotEmpty() const { return Len() != 0; }

	void Truncate (long newlen);

	int Compare (const FString &other) const { return strcmp (CharsPtr->c_str(), other.CharsPtr->c_str()); }
	int Compare (const char *other) const { return strcmp (CharsPtr->c_str(), other); }
	int Compare(const FString &other, int len) const { return strncmp(CharsPtr->c_str(), other.CharsPtr->c_str(), len); }
	int Compare(const char *other, int len) const { return strncmp(CharsPtr->c_str(), other, len); }

	int CompareNoCase (const FString &other) const { return stricmp (CharsPtr->c_str(), other.CharsPtr->c_str()); }
	int CompareNoCase (const char *other) const { return stricmp (CharsPtr->c_str(), other); }
	int CompareNoCase(const FString &other, int len) const { return strnicmp(CharsPtr->c_str(), other.CharsPtr->c_str(), len); }
	int CompareNoCase(const char *other, int len) const { return strnicmp(CharsPtr->c_str(), other, len); }

protected:
	static int FormatHelper (void *data, const char *str, int len);

	void AttachToOther (const FString &other);

	void CopyOnWrite();

	std::shared_ptr<std::string> CharsPtr;
	int LockCount;

private:
	// Prevent these from being called as current practices are to use Compare.
	// Without this FStrings will be accidentally compared against char* ptrs.
	bool operator == (const FString &illegal) const;
	bool operator != (const FString &illegal) const;
	bool operator < (const FString &illegal) const;
	bool operator > (const FString &illegal) const;
	bool operator <= (const FString &illegal) const;
	bool operator >= (const FString &illegal) const;
};

namespace StringFormat
{
	enum
	{
		// Format specification flags
		F_MINUS		= 1,
		F_PLUS		= 2,
		F_ZERO		= 4,
		F_BLANK		= 8,
		F_HASH		= 16,

		F_SIGNED	= 32,
		F_NEGATIVE	= 64,
		F_ZEROVALUE	= 128,
		F_FPT		= 256,

		// Format specification size prefixes
		F_HALFHALF	= 0x1000,	// hh
		F_HALF		= 0x2000,	// h
		F_LONG		= 0x3000,	// l
		F_LONGLONG	= 0x4000,	// ll or I64
		F_BIGI		= 0x5000,	// I
		F_PTRDIFF	= 0x6000,	// t
		F_SIZE		= 0x7000,	// z
	};
	typedef int (*OutputFunc)(void *data, const char *str, int len);

	int VWorker (OutputFunc output, void *outputData, const char *fmt, va_list arglist);
	int Worker (OutputFunc output, void *outputData, const char *fmt, ...);
};

#undef PRINTFISH

// FName inline implementations that take FString parameters

inline FName::FName(const FString &text) { Index = NameData.FindName (text, text.Len(), false); }
inline FName::FName(const FString &text, bool noCreate) { Index = NameData.FindName (text, text.Len(), noCreate); }
inline FName &FName::operator = (const FString &text) { Index = NameData.FindName (text, text.Len(), false); return *this; }
inline FName &FNameNoInit::operator = (const FString &text) { Index = NameData.FindName (text, text.Len(), false); return *this; }


#endif
