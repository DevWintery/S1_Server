#include "pch.h"
#include "Vector.h"

float FVector::Size() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z);
}

FVector FVector::Normalize() const
{
	float length = Size();
	return FVector(X / length, Y / length, Z / length);
}

float FVector::Dist(const FVector& V1, const FVector& V2)
{
	return FMath::Sqrt(FMath::DistSquared(V1, V2));
}

float FVector::Distance(const FVector& V1, const FVector& V2)
{
	return Dist(V1, V2);
}
