#pragma once

#include "IocpCore.h"
#include "IocpObject.h"
#include "NetAddress.h"

/*-----------------
* Listener
*
* Listen Socket Å¬·¡½º
------------------- */

class AcceptEvent;
class ServerService;

class Listener : public IocpObject
{
public:
	Listener();
	~Listener();

public:
	bool StartAccept(shared_ptr<ServerService> service);
	void CloseSocket();
	
public:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _acceptEvents;
	shared_ptr<ServerService> _service;
};

