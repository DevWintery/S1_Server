#pragma once

/*-----------------
* IocpObject
*
* Iocp Event�� ��Ͻ�ų �� �ִ� ��ü
------------------- */

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;

	/* Iocp�� ���ö� iocpEvent�� �Ǵ��Ͽ� �̺�Ʈ�� ������ */
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};