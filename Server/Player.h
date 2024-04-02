#pragma once

#include "Creature.h"

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

public:
	weak_ptr<GameSession>& GetSession() { return _session; }
	void SetSession(const weak_ptr<GameSession>& session) { _session = session;}

public:
	virtual void Update() override;

private:
	weak_ptr<GameSession> _session;
};

