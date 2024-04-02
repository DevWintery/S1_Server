#pragma once

/*-----------------
* IocpEvent
*
* Iocp에 들어올때 실행하는 이벤트
------------------- */

class IocpObject;
class Session;
class SendBuffer;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

public:
	/* Overlapped 변수를 초기화하는 함수 */
	void Init();

public:
	__forceinline EventType GetEventType() { return _eventType; }
	__forceinline shared_ptr<IocpObject> GetOwner() { return _owner; }
	__forceinline void SetOwner(shared_ptr<IocpObject> owner) { _owner = owner; }

private:
	EventType _eventType;
	shared_ptr<IocpObject> _owner;
};


class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) { }
};

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) { }
};

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

public:
	__forceinline void SetSession(shared_ptr<Session> session ) { _session = session; }
	__forceinline shared_ptr<Session> GetSession() { return _session; }
private:
	shared_ptr<Session>	_session = nullptr;
};

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) { }
};

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) { }

	vector<shared_ptr<SendBuffer>> sendBuffers;
};