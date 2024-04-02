#pragma once

/*-----------------
* IocpCore
*
* Iocp�� �����ϴ� Ŭ���� IocpObject�� �̟G�Ͽ� �̺�Ʈ�� �����Ŵ
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

