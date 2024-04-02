#include "pch.h"
#include "Object.h"
#include "Room.h"

Object::Object()
{
	objectInfo = new Protocol::ObjectInfo();
	posInfo = new Protocol::PosInfo();
	objectInfo->set_allocated_pos_info(posInfo);
}

Object::~Object()
{
	delete posInfo;
	delete objectInfo;
}

const FVector Object::GetPosVector()
{
	return FVector(GetPos()->x(), GetPos()->y(), GetPos()->z());
}

void Object::SetPos(const FVector& pos)
{
	posInfo->set_x(pos.X);
	posInfo->set_y(pos.Y);
	posInfo->set_z(pos.Z);
}
