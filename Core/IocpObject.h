#pragma once

/*-----------------
* IocpObject
*
* Iocp Event에 등록시킬 수 있는 객체
------------------- */

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;

	/* Iocp에 들어올때 iocpEvent를 판단하여 이벤트를 실행함 */
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};