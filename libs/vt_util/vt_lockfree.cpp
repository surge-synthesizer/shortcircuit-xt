#include "vt_lockfree.h"

vt_LockFree::vt_LockFree(size_t EntrySize, unsigned int Entries, unsigned int Readers)
{
	data = _mm_malloc(Entries*EntrySize,16);
	WriterPos = 0;
	NumReaders = Readers;
	NumEntries = Entries;
	this->EntrySize = EntrySize;

	for(unsigned int i=0; i<Readers; i++)
	{
		ReaderPos[i] = 0;
	}
}
vt_LockFree::~vt_LockFree()
{
	_mm_free(data);
}

void vt_LockFree::WriteBlock(void *srcData)
{
	memcpy((char*)data + WriterPos*EntrySize, srcData, EntrySize);

	unsigned int tmp = WriterPos;

	tmp++;
	if(tmp >= NumEntries) tmp = 0;
	// ACHTUNG! WritePos får absolut inte skrivas till minnet med fel värde i ett mellansteg

	WriterPos = tmp;
}

void* vt_LockFree::ReadBlock(unsigned int Reader)
{
	if(ReaderPos[Reader] == WriterPos) return 0;	// Nothing new in the queue

	unsigned int rp = ReaderPos[Reader];
	ReaderPos[Reader]++;
	if(ReaderPos[Reader] >= NumEntries) ReaderPos[Reader] = 0;

	return (char*)data + EntrySize*rp;
}

void vt_LockFree::SynchronizeReader(unsigned int Reader)
{
	ReaderPos[Reader] = WriterPos;
}