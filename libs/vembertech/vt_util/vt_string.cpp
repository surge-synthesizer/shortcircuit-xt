#if WINDOWS
#include "vt_string.h"
#include <assert.h>
#include <string.h>
#include <wchar.h>
#include <Windows.h>


//-------------------------------------------------------------------------------------------------------

void vtCopyString(char* Destination, const char* Source, size_t Length)
{
	assert(strlen(Source) < Length);

	//strncpy_s(Destination, Length, Source, _TRUNCATE);
	strncpy(Destination, Source, Length);

	Destination[Length-1] = 0;
}

//-------------------------------------------------------------------------------------------------------

void vtCopyStringW(wchar_t* Destination, const wchar_t* Source, size_t Length)
{
	assert(wcslen(Source) < Length);

	wcsncpy_s(Destination, Length, Source, _TRUNCATE);

	Destination[Length-1] = 0;
}

//-------------------------------------------------------------------------------------------------------

void vtWStringToString(char* Destination, const wchar_t* Source, size_t Length)
{
	assert(wcslen(Source) < Length);

	WideCharToMultiByte(CP_UTF8, 0, Source, -1, Destination, (int)Length,0,0);				
}

//-------------------------------------------------------------------------------------------------------

void vtStringToWString(wchar_t* Destination, const char* Source, size_t Length)
{	
	assert(strlen(Source) < Length);

	MultiByteToWideChar(CP_UTF8, 0, Source, -1, Destination, (int)Length);	
}

//-------------------------------------------------------------------------------------------------------
//
//		wstring helpers
//
//-------------------------------------------------------------------------------------------------------

std::wstring ToW(const char* Source)
{
	size_t Length = strlen(Source);

	wchar_t* TempWString = 
		reinterpret_cast<wchar_t*>(malloc((Length + 1) * sizeof(wchar_t)));

	vtStringToWString(TempWString, Source, Length + 1);

	std::wstring Result = TempWString;

	free(TempWString);

	return Result;
}

//-------------------------------------------------------------------------------------------------------

std::wstring ToW(int Value)
{	
	wchar_t TempW[16];
	swprintf_s(TempW, 16, L"%i", Value);
	std::wstring Result = TempW;	

	return Result;
}


//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
#else
void vtCopyString( char*, char const*, unsigned long ) {}
void vtCopyStringW( wchar_t*, wchar_t const *, unsigned long ) {}
void vtStringToWString(wchar_t*, char const*, unsigned long) {}
void vtWStringToString( char*, wchar_t const*, unsigned long) {}
#endif
