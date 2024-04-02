#pragma once

class SendBufferChunk;

/*-----------------
* SendBuffer
*
------------------- */
class SendBuffer
{
public:
	SendBuffer(shared_ptr<SendBufferChunk> owner, BYTE* buffer, uint32 allocSize);
	~SendBuffer();

public:
	__forceinline BYTE* Buffer() { return _buffer; }
	__forceinline uint32 AllocSize() { return _allocSize; }
	__forceinline uint32 WriteSize() { return _writeSize; }

public:
	void Close(uint32 writeSize);

private:
	BYTE* _buffer;
	uint32 _allocSize = 0;
	uint32 _writeSize = 0;
	shared_ptr<SendBufferChunk> _owner;
};

/*-----------------
* SendBufferChunk
*
* SendBuffer를 재활용하기 위한 클래스
* SendBuffer를 대용량으로 할당한뒤 Cursor를 이용하여 이용한다.
------------------- */

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum
	{
		SEND_BUFFER_CHUNK_SIZE = 6000
	};

public:
	SendBufferChunk();
	~SendBufferChunk();

public:
	void Reset();
	shared_ptr<SendBuffer> Open(uint32 allocSize);
	void Close(uint32 writeSize);

public:
	bool IsOpen() { return _open; }
	BYTE* Buffer() { return &_buffer[_usedSize]; }
	uint32 FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }

private:
	array<BYTE, SEND_BUFFER_CHUNK_SIZE> _buffer = {};
	bool _open = false;
	uint32 _usedSize = 0;
};

class SendBufferManager
{
public:
	shared_ptr<SendBuffer> Open(uint32 size);

private:
	shared_ptr<SendBufferChunk> Pop();
	void Push(shared_ptr<SendBufferChunk> buffer);

	static void PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	vector<shared_ptr<SendBufferChunk>> _sendBufferChunks;
};
