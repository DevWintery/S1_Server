#pragma once

#include "IocpEvent.h"
#include "NetAddress.h"
#include "IocpObject.h"
#include "RecvBuffer.h"

/*-----------------
* Session
*
* 통신을 담당하는 세션 클래스
------------------- */

class Service;

class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

/* 외부에서 사용 */
public:	
	void Send(shared_ptr<SendBuffer> sendBuffer);
	bool Connect();
	void Disconnect(const WCHAR* cause);

/* 정보 관련 */
public:
	__forceinline shared_ptr<Service>	GetService() { return _service.lock(); }
	__forceinline void SetService(shared_ptr<Service> service) { _service = service; }
	__forceinline void SetNetAddress(NetAddress address) { _netAddress = address; }
	__forceinline NetAddress GetAddress() { return _netAddress; }
	__forceinline SOCKET GetSocket() { return _socket; }
	__forceinline bool IsConnected() { return _connected; }
	__forceinline shared_ptr<Session>	GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

/* 인터페이스 구현 */
private:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

/* 전송 관련 */
private:
	bool RegisterConnect();
	bool RegisterDisconnect();
	void RegisterRecv();
	void RegisterSend();

	void ProcessConnect();
	void ProcessDisconnect();
	void ProcessRecv(int32 numOfBytes);
	void ProcessSend(int32 numOfBytes);

	void HandleError(int32 errorCode);

/* 컨텐츠 코드에서 재정의 */
protected:
	virtual void OnConnected() { }
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 len) { }
	virtual void OnDisconnected() { }

private:
	weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	NetAddress _netAddress = {};
	atomic<bool> _connected = false;

private:
	USE_LOCK;

	/* 수신 관련 */
	RecvBuffer _recvBuffer;

	/* 송신 관련 */
	queue<shared_ptr<SendBuffer>> _sendQueue;
	atomic<bool> _sendRegistered = false;

private:
	/* IocpEvent 재사용 */
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;
};
/*-----------------
* PacketSession
*
* Packet 무결성을 위한 세션
------------------- */

struct PacketHeader
{
	uint16 size;
	uint16 id; // 프로토콜ID (ex. 1=로그인, 2=이동요청)
};

class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	shared_ptr<PacketSession> GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32 OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) abstract;
};