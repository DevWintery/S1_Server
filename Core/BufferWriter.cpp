#include "pch.h"
#include "BufferWriter.h"

BufferWriter::BufferWriter()
{

}

BufferWriter::BufferWriter(BYTE* buffer, uint32 size, uint32 pos /*= 0*/):
	_buffer(buffer), _size(size), _pos(pos)
{

}

BufferWriter::~BufferWriter()
{

}

bool BufferWriter::Write(void* src, uint32 len)
{
	/* 남은 사이즈보다 더 크면 리턴 */
	if (FreeSize() < len)
		return false;

	/* 버퍼 채우기 */
	::memcpy(&_buffer[_pos], src, len);
	_pos += len;
	return true;
}
