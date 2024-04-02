#pragma once

/*-----------------
* BufferWriter
*
* Buffer 쓰기를 위한 유틸 클래스
* Cursor를 사용하여 Buffer를 채워넣는다.
------------------- */

class BufferWriter
{
public:
	BufferWriter();
	BufferWriter(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferWriter();

public:
	template<typename T>
	bool Write(T* src) { return Write(src, sizeof(T)); }
	bool Write(void* src, uint32 len);

	template<typename T>
	T* Reserve(uint16 count = 1);

	template<typename T>
	BufferWriter& operator<<(T&& src);

public:
	__forceinline BYTE* Buffer() { return _buffer; }
	__forceinline uint32 Size() { return _size; }
	__forceinline uint32 WriteSize() { return _pos; }
	__forceinline uint32 FreeSize() { return _size - _pos; }

private:
	BYTE* _buffer = nullptr;
	uint32	_size = 0;
	uint32	_pos = 0;
};

template<typename T>
T* BufferWriter::Reserve(uint16 count /*= 1*/)
{
	/* 남은 사이즈보다 더 크면 리턴 */
	if (FreeSize() < (sizeof(T) * count))
	{
		return nullptr;
	}

	/* 현재 커서위치를 반환 */
	T* ret = reinterpret_cast<T*>(&_buffer[_pos]);
	_pos += (sizeof(T) * count);

	return ret;
}

template<typename T>
BufferWriter& BufferWriter::operator<<(T&& src)
{
	using DataType = std::remove_reference_t<T>;
	*reinterpret_cast<DataType*>(&_buffer[_pos]) = std::forward<DataType>(src);
	_pos += sizeof(T);
	return *this;
}

