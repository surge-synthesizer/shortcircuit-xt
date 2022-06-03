#error THIS IS NOT USED
#if INCLUDE_REX

#include "../common/rex.h"
#include "globals.h"
#include "logfile.h"
#include "resampling.h"
#include "sample.h"
#include "sampler_state.h"
#include <assert.h>
#include <windows.h>
#include <mmreg.h>

using namespace REX;

void PrintREXError(REXError error);
#define kREXErrorTextMaxLen 255

bool sample::load_recycle(const char *fname)
{
    REXError result;
    REXHandle handle = 0;
    REXInfo info;
    REXCreatorInfo creatorInfo;
    size_t fileSize = 0;
    char *fileBuffer = 0;

    char filename[256];
    conf->decode_path(fname, filename);

    // load recycle .dll
    result = REX::REXLoadDLL();
    if (result != kREXError_NoError)
    {
        PrintREXError(result);
        // write_log("file	io:	Could not load and/or init recycle .dll!");
        return false;
    }

    // load file into buffer
    FILE *inputFile = 0;
    size_t bytesRead = 0;

    inputFile = fopen(filename, "rb");
    if (inputFile == 0)
    {
        write_log("file	io:	Could not open file");
        REXUnloadDLL();
        return false;
    }

    /* Find out file size. */
    if (fseek(inputFile, 0, SEEK_END) != 0)
    {
        fclose(inputFile);
        write_log("file	io:	Unable to seek end of file");
        REXUnloadDLL();
        return false;
    }
    fileSize = ftell(inputFile);
    rewind(inputFile);

    /* Allocate buffer for file and read file into it. */
    fileBuffer = (char *)malloc(fileSize * sizeof(char));
    if (fileBuffer == 0)
    {
        fclose(inputFile);
        write_log("file	io: out of memory!!");
        REXUnloadDLL();
        return false;
    }

    bytesRead = fread(fileBuffer, sizeof(char), fileSize, inputFile);
    if (bytesRead != fileSize)
    {
        write_log("file	io: unable to read file");
        free(fileBuffer);
        fclose(inputFile);
        REXUnloadDLL();
        return false;
    }
    fclose(inputFile);

    /*
    Create REX object from buffered file.
    */
    result = REXCreate(&handle, fileBuffer, fileSize, 0, 0);
    if (result == kREXError_NoError)
    {
        /*
        Print information about the file, then extract slices and render as a single bar.
        */
        result = REXGetInfo(handle, sizeof(REXInfo), &info);
        if (result == kREXError_NoError)
        {
            this->sample_rate = info.fSampleRate;
            this->channels = info.fChannels;
            REXSliceInfo sliceInfo;

            n_slices = (int)min(info.fSliceCount, max_hitpoints);
            float ppqratio = (float)info.fSampleRate * 125.f / (32.f * (float)info.fTempo);
            //			float num_samples = (float)info.fPPQLength * ppqratio;

            if (!n_slices)
            {
                write_log("REX error: This ReCycle file does not contain any active slices.");
                goto bailout;
            }

            result = REXGetSliceInfo(handle, n_slices - 1, sizeof(REXSliceInfo), &sliceInfo);
            if (result != kREXError_NoError)
            {
                PrintREXError(result);
                goto bailout;
            }
            float num_samples =
                ppqratio * (float)sliceInfo.fPPQPos + (float)sliceInfo.fSampleLength;

            slice_start = new int[n_slices];
            slice_end = new int[n_slices];

            this->sample_length = (int)ceil(num_samples);

            // allocate memory;
            uint32 samplesizewithmargin =
                this->sample_length + 2 * FIRipol_N + BLOCK_SIZE + FIRoffset;
            sample_data = new float[samplesizewithmargin];
            int i;
            for (i = 0; i < samplesizewithmargin; i++)
                sample_data[i] = 0.0f;
            if (channels == 2)
            {
                sample_data_R = new float[samplesizewithmargin];
                for (i = 0; i < samplesizewithmargin; i++)
                    sample_data_R[i] = 0.0f;
            }
            else
                sample_data_R = 0;

            // do another run and load the samples this time

            for (i = 0; i < n_slices; i++)
            {
                result = REXGetSliceInfo(handle, i, sizeof(REXSliceInfo), &sliceInfo);
                if (result != kREXError_NoError)
                {
                    PrintREXError(result);
                    goto bailout;
                }
                int samplepos = (int)((float)ppqratio * sliceInfo.fPPQPos);
                slice_start[i] = samplepos;
                slice_end[i] = samplepos + sliceInfo.fSampleLength;

                float *ob[2];
                ob[0] = &sample_data[samplepos];
                if (this->channels == 2)
                    ob[1] = &sample_data_R[samplepos];
                else
                    ob[1] = 0;

                int samples_left = this->sample_length - samplepos;

                result = REXRenderSlice(handle, i, min(sliceInfo.fSampleLength, samples_left), ob);
                if (result != kREXError_NoError)
                {
                    PrintREXError(result);
                    goto bailout;
                }
                // samplepos += sliceInfo.fSampleLength;
            }
        }
    }
    else
    {
        PrintREXError(result);
        goto bailout;
    }

    // Free all resources used by the REX object.
    REXDelete(&handle);
    free(fileBuffer);

    this->sample_loaded = true;
    this->inv_sample_rate = 1 / this->sample_rate;
    sprintf(this->filename, "%s", filename);
    this->smpl_present = false;

    REX::REXUnloadDLL();
    return true;

bailout:
    // Free all resources used by the REX object.
    REXDelete(&handle);
    free(fileBuffer);
    if (sample_data)
        delete sample_data;
    if (sample_data_R)
        delete sample_data_R;
    REX::REXUnloadDLL();
    return false;
}

void GetErrorText(REXError error, char *text, long size)
{
    assert(size >= kREXErrorTextMaxLen);

    switch (error)
    {
    /*
            Error codes that can be expected at run-time, but are not really errors.
    */
    case kREXError_NoError:
        sprintf(text, "%s", "No Error. (You shouldn't show an error text in this case!)");
        break;
    case kREXError_OperationAbortedByUser:
        sprintf(text, "%s",
                "Operation was aborted by user. (You shouldn't show an error text in this case!)");
        break;
    case kREXError_NoCreatorInfoAvailable:
        sprintf(text, "%s",
                "There is no optional creator-supplied information in this file. (You shouldn't "
                "show an error text in this case!)");
        break;

    /*
            Errors that could be expected at run-time.
    */
    case kREXError_NotEnoughMemoryForDLL:
        sprintf(text, "%s",
                "Couldn't load the REX DLL because there is too little free memory. Quit some "
                "other application, then try again.");
        break;
    case kREXError_DLLTooOld:
        sprintf(text, "%s",
                "Couldn't load the REX DLL because it is too old. Please update the DLL, then try "
                "again.");
        break;
    case kREXError_DLLNotFound:
        sprintf(text, "%s",
                "The REX DLL could not be found. Maybe REX is not installed on this system?");
        break;
    case kREXError_APITooOld:
        sprintf(
            text, "%s",
            "The REX DLL could not be used because it is newer than this application is expecting. "
            "Please update the application, or install an older version of the REX DLL.");
        break;
    case kREXError_UnableToLoadDLL:
        sprintf(text, "%s",
                "Couldn't load the REX DLL. The DLL file may be corrupt. Please re-install REX, "
                "then try again. If it still doesn't work after that, call tech support.");
        break;
    case kREXError_OutOfMemory:
        sprintf(text, "%s",
                "The operation could not be completed because there is too little free memory. "
                "Quit some other application, then try again.");
        break;
    case kREXError_FileCorrupt:
        sprintf(text, "%s", "The format of this file is unknown or the file is corrupt.");
        break;
    case kREXError_REX2FileTooNew:
        sprintf(text, "%s",
                "This REX2 file was created by a later version of ReCycle and cannot be loaded "
                "with this version of the REX DLL. Please update the DLL, then try again.");
        break;
    case kREXError_FileHasZeroLoopLength:
        sprintf(text, "%s",
                "This ReCycle file cannot be used because its \"Bars\" and \"Beats\" settings have "
                "not been set.");
        break;
    case kREXError_OSVersionNotSupported:
        sprintf(text, "%s",
                "Couldn't load the REX DLL because it requires a newer version of Windows or Mac "
                "OS than is installed on this machine.");
        break;

    /*
            Errors that won't happen at run-time, if your code complies to the REX API.
    */
    case kREXImplError_DLLNotLoaded:
        sprintf(text, "%s",
                "The REX DLL has not been loaded. You must call REXLoadDLL() before any other "
                "function in the REX API.");
        break;
    case kREXImplError_DLLAlreadyLoaded:
        sprintf(text, "%s",
                "The REX DLL is already loaded. You may not nest calls to REXLoadDLL().");
        break;
    case kREXImplError_InvalidHandle:
        sprintf(text, "%s",
                "Invalid handle. The handle you supplied to the function does not refer to a REX "
                "object currently in memory.");
        break;
    case kREXImplError_InvalidSize:
        sprintf(text, "%s",
                "Invalid size. The fSize field of the REXInfo or REXSliceInfo structure was not "
                "set up correctly. Please set the field to \"sizeof(REXInfo)\" or "
                "\"sizeof(REXSliceInfo)\" prior to the function call.");
        break;
    case kREXImplError_InvalidArgument:
        sprintf(text, "%s",
                "Invalid argument. One of the parameters you supplied is out of range. Please "
                "check the documentation on this function for more specific help.");
        break;
    case kREXImplError_InvalidSlice:
        sprintf(text, "%s",
                "Invalid slice. The slice number you have supplied is out of range. Check the "
                "fSliceCount member of the REXInfo structure for this REX object, to see how many "
                "slices this object has.");
        break;
    case kREXImplError_InvalidSampleRate:
        sprintf(text, "%s",
                "Invalid sample rate. The sample rate must be in the range 11025 Hz - 1000000 Hz, "
                "inclusive.");
        break;
    case kREXImplError_BufferTooSmall:
        sprintf(text, "%s",
                "The buffer you have supplied is too small for this slice. Check the documentation "
                "on REXRenderSlice() for an example on how to calculate the needed buffer size.");
        break;
    case kREXImplError_IsBeingPreviewed:
        sprintf(text, "%s",
                "This REX object is being previewed. You must stop the preview by calling "
                "REXStopPreview() before previewing can be restarted or before changing the output "
                "sample rate.");
        break;
    case kREXImplError_NotBeingPreviewed:
        sprintf(text, "%s",
                "This object is not currently being previewed. You must not call REXRenderBatch() "
                "or REXStopPreview() on a REX object unless preview has been started.");
        break;
    case kREXImplError_InvalidTempo:
        sprintf(
            text, "%s",
            "Invalid tempo. The preview tempo must be in the range 20 BPM - 450 BPM, inclusive.");
        break;

    /*
            Errors that indicate problems in the API.
    */
    case kREXError_Undefined:
        sprintf(text, "%s", "Undefined error. If this happens, you've found a bug in the REX DLL.");
        break;
    default:
        assert(0);
        break;
    }
}

/*
        PrintREXError()

        Gets the error text for a REX error and prints it.
*/

void PrintREXError(REXError error)
{
    char temp[kREXErrorTextMaxLen];
    char temp2[kREXErrorTextMaxLen];
    GetErrorText(error, temp, kREXErrorTextMaxLen);
    // printf("%s\n", temp);
    sprintf(temp2, "REX error :%s", temp);
    write_log(temp2);
}

#endif
