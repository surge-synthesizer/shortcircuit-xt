//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4       $Date: 2006/01/12 09:05:31 $
//
// Category     : VST 2.x Classes
// Filename     : vstplugmain.cpp
// Created by   : Steinberg Media Technologies
// Description  : VST Plug-In Main Entry
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include "audioeffect.h"
#include <Windows.h>
#include <cwchar>

//------------------------------------------------------------------------
/** Must be implemented externally. */
extern AudioEffect *createEffectInstance(audioMasterCallback audioMaster);

extern "C"
{

#if defined(__GNUC__)
#define VST_EXPORT __attribute__((visibility("default")))
#else
#define VST_EXPORT
#endif

    //------------------------------------------------------------------------

    void AppInvalidParameterHandler(const wchar_t *expression, const wchar_t *function,
                                    const wchar_t *file, unsigned int line, uintptr_t pReserved)
    {
        wchar_t err[1024];
        swprintf(err,
                 L"Invalid parameter detected in function %s. File: %s Line: %d\n"
                 L"Expression: %s\n",
                 function, file, line, expression);

        MessageBoxW(::GetActiveWindow(), err, L"Invalid Parameter (0xc000000d)",
                    MB_OK | MB_ICONERROR);

        // Cause a Debug Breakpoint.
        DebugBreak();
    }

    //------------------------------------------------------------------------
    /** Prototype of the export function main */
    //------------------------------------------------------------------------
    VST_EXPORT AEffect *VSTPluginMain(audioMasterCallback audioMaster)
    {
#ifdef _DEBUG
        // activate debug memory check
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif

        // Every once and awhile something looks at a std::vector or some other
        // CRT/STL construct that throws an exception when an invalid parameter
        // is detected.  In this case we should dump whatever information we
        // can and then bail.  When we bail we should dump as much data as
        // possible.

        // funkar inte i releasemode
        //_set_invalid_parameter_handler(AppInvalidParameterHandler);

        // Get VST Version
        if (!audioMaster(0, audioMasterVersion, 0, 0, 0, 0))
            return 0; // old version

        // Create the AudioEffect
        AudioEffect *effect = createEffectInstance(audioMaster);
        if (!effect)
            return 0;

        return effect->getAeffect();
    }

// support for old hosts not looking for VSTPluginMain
#if (TARGET_API_MAC_CARBON && __ppc__)
    VST_EXPORT AEffect *main_macho(audioMasterCallback audioMaster)
    {
        return VSTPluginMain(audioMaster);
    }
#elif WIN32
    VST_EXPORT AEffect *MAIN(audioMasterCallback audioMaster) { return VSTPluginMain(audioMaster); }
#elif BEOS
    VST_EXPORT AEffect *main_plugin(audioMasterCallback audioMaster)
    {
        return VSTPluginMain(audioMaster);
    }
#endif

} // extern "C"

//------------------------------------------------------------------------
#if WIN32
#include <windows.h>
void *hInstance;
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    hInstance = hInst;
    return 1;
}
#endif
