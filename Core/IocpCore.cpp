#include "pch.h"
#include "IocpCore.h"
#include "IocpObject.h"
#include "IocpEvent.h"

IocpCore::IocpCore()
{
	/* Iocp 초기화 */
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(shared_ptr<IocpObject> iocpObject)
{
	return ::CreateIoCompletionPort(iocpObject->GetHandle(),_iocpHandle, 0, 0); //마지막인자를 0으로 줄시 CPU만큼의 쓰레드 할당
}

bool IocpCore::Dispatch(uint32 timeoutMs /*= INFINITE*/)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT & key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		shared_ptr<IocpObject> iocpObject = iocpEvent->GetOwner();
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		/* TODO : Logging을 위하여 case분리 */
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			shared_ptr<IocpObject> iocpObject = iocpEvent->GetOwner();
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
