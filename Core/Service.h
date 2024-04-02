#pragma once

#include"NetAddress.h"
#include"IocpCore.h"
#include "Listener.h"

enum class ServiceType : uint8
{
	Server,
	Client
};

/*-----------------
* Service
*
* Listener, IocpCore, Session 등을 관리하는 클래스
------------------- */

using SessionFactory = function<shared_ptr<Session>(void)>;


class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~Service();

public:
	virtual bool Start() abstract;
	virtual void CloseService();

/* Sessions */
public:
	shared_ptr<Session> CreateSession();
	void AddSession(shared_ptr<Session> session);
	void ReleaseSession(shared_ptr<Session> session);

	__forceinline int32 GetCurrentSessionCount() { return _sessionCount; }
	__forceinline int32 GetMaxSessionCount() { return _maxSessionCount; }
	__forceinline void SetSessionFactory(SessionFactory func) { _sessionFactory = func; }

public:
	__forceinline bool CanStart() { return _sessionFactory != nullptr; }

public:
	ServiceType	GetServiceType() { return _type; }
	NetAddress	GetNetAddress() { return _netAddress; }
	shared_ptr<IocpCore>& GetIocpCore() { return _iocpCore; }

protected:
	USE_LOCK;
	ServiceType	_type;
	NetAddress	_netAddress = {};
	shared_ptr<IocpCore> _iocpCore;

	set<shared_ptr<Session>> _sessions;
	int32 _sessionCount = 0;
	int32 _maxSessionCount = 0;
	SessionFactory _sessionFactory;
};

/*-----------------
* ClientService
*
* Client용 Serivce 클래스
------------------- */

class ClientService : public Service
{
public:
	ClientService(NetAddress targetAddress, shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ClientService() {}

	virtual bool Start() override;
};


/*-----------------
* ServerService
*
* Server용 Serivce 클래스
------------------- */

class ServerService : public Service
{
public:
	ServerService(NetAddress address, shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;
	virtual void CloseService() override;

private:
	shared_ptr<Listener> _listener = nullptr;
};