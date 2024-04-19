#pragma once

class Room;

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

public:
	Protocol::PosInfo* GetPos() { return posInfo; }
	Protocol::ObjectInfo* GetInfo() { return objectInfo; }

	virtual const FVector GetPosVector();
	virtual void SetPos(const FVector& pos);

public:
	bool IsPlayer() { return _isPlayer; }
	bool IsDie() { return _isDie; }

public:
	virtual void Initialize() { }
	virtual void Update() { }
	virtual void TakeDamage(float damage);

protected:
	bool _isPlayer = false;

	//TEMP
	bool _isDie = false;
	float _hp = 100.f;

protected:
	Protocol::ObjectInfo* objectInfo;
	Protocol::PosInfo* posInfo;

public:
	atomic<weak_ptr<Room>> room;
};
