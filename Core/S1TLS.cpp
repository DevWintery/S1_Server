#include "pch.h"
#include "S1TLS.h"

thread_local uint32 LThreadID = 0;
thread_local uint64				LEndTickCount = 0;

thread_local shared_ptr<SendBufferChunk> LSendBufferChunk;
thread_local JobQueue* LCurrentJobQueue = nullptr;