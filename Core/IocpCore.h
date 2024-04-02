#pragma once

/*-----------------
* IocpCore
*
* Iocp를 관리하는 클래스 IocpObject를 이욯하여 이벤트를 실행시킴
------------------- */

class IocpObject;

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

public:
	HANDLE GetHandle() { return _iocpHandle; }
	
	bool Register(shared_ptr<IocpObject> iocpObject);
	bool Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle;
};

