#error THIS CODE DOES NOT COMPILE

#if 0

#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h" // must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);
#endif

#include <Windows.h>

class MiniDumper
{
  private:
    static LPCSTR m_szAppName;

    static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo) { return 0; }

  public:
    MiniDumper(LPCSTR szAppName) {}
};
