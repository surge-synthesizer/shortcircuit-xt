#include <cstddef>
#include <cstdlib>

const int max_readers = 16;

// Lock-free thread-safe FIFO buffer class that allows a single writer and multiple readers
class vt_LockFree
{
public:
	vt_LockFree(size_t EntrySize, unsigned int Entries, unsigned int Readers);
	~vt_LockFree();
	void WriteBlock(void *Data);
	void* ReadBlock(unsigned int Reader=0);
	void SynchronizeReader(unsigned int Reader=0);	// Ignore all previous messages
private:
	void *data;
	size_t EntrySize;
	unsigned int NumEntries,NumReaders;
	unsigned int ReaderPos[max_readers];
	unsigned int volatile WriterPos;
};
