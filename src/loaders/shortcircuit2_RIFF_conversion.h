#pragma once
#include "sampler_state.h"
#include "shortcircuit2_RIFF_format.h"
#include "synthesis/modmatrix.h"
#include <vt_dsp/endian.h>

int RIFF_FILTER_Type_To_Internal(int);
int RIFF_FILTER_Type_From_Internal(int);

unsigned char Playmode_To_Internal(unsigned char);
unsigned char Playmode_From_Internal(unsigned char);

void ReadChunkFltD(RIFF_FILTER *F, filterstruct *f);
void WriteChunkFltD(RIFF_FILTER *F, filterstruct *f);
void ReadChunkFltB(RIFF_FILTER_BUSSEX *F, sample_multi *m, int id);
void WriteChunkFltB(RIFF_FILTER_BUSSEX *F, sample_multi *m, int id);
void ReadChunkAuxB(RIFF_AUX_BUSS *E, aux_buss *e);
void WriteChunkAuxB(RIFF_AUX_BUSS *E, aux_buss *e);
void ReadChunkMMen(RIFF_MM_ENTRY *E, mm_entry *e, modmatrix *mm);
void WriteChunkMMen(RIFF_MM_ENTRY *E, mm_entry *e, modmatrix *mm);
void ReadChunkNCen(RIFF_NC_ENTRY *E, nc_entry *e, modmatrix *mm);
void WriteChunkNCen(RIFF_NC_ENTRY *E, nc_entry *e, modmatrix *mm);
void ReadChunkSLFO(RIFF_LFO *E, steplfostruct *e);
void WriteChunkSLFO(RIFF_LFO *E, steplfostruct *e);
void ReadChunkAHDS(RIFF_AHDSR *E, envelope_AHDSR *e);
void WriteChunkAHDS(RIFF_AHDSR *E, envelope_AHDSR *e);
void ReadChunkHtPt(RIFF_HITPOINT *E, hitpoint *e);
void WriteChunkHtPt(RIFF_HITPOINT *E, hitpoint *e);
void ReadChunkZonD(RIFF_ZONE *Z, sample_zone *z);
void WriteChunkZonD(RIFF_ZONE *Z, sample_zone *z, int SampleID);
void ReadChunkParD(RIFF_PART *P, sample_part *p);
void WriteChunkParD(RIFF_PART *P, sample_part *p);
void ReadChunkCtrD(RIFF_CONTROLLER *e, sample_part *p, int id);
void WriteChunkCtrD(RIFF_CONTROLLER *e, sample_part *p, int id);