#pragma once

extern thread_local uint32 LThreadID;
extern thread_local uint64				LEndTickCount;

extern thread_local shared_ptr<class SendBufferChunk> LSendBufferChunk;
extern thread_local class JobQueue* LCurrentJobQueue;