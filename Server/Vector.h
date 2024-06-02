#pragma once

struct FVector
{
	FVector():
		X(0.f), Y(0.f), Z(0.f)
	{}

	FVector(float X, float Y, float Z) :
		X(X), Y(Y), Z(Z)
	{}

	float X;
	float Y;
	float Z;

	float Size() const;
	FVector Normalize() const;
	bool IsZero() { return Size() >= 0.f; }

	static float Dist(const FVector& V1, const FVector& V2);
	static float Distance(const FVector& V1, const FVector& V2);
	static float Dot(const FVector& V1, const FVector& V2);
	static FVector Zero() { return FVector(0.f, 0.f, 0.f); }
	

	FVector operator+(const FVector& V) const
	{
		return FVector(X + V.X, Y + V.Y, Z + V.Z);
	}

	FVector operator-(const FVector& V) const
	{
		return FVector(X - V.X, Y - V.Y, Z - V.Z);
	}

	FVector operator/(float Scale) const
	{
		const float RScale = float(1) / Scale;
		return FVector(X * RScale, Y * RScale, Z * RScale);
	}

	FVector operator*(float Scale) const
	{
		return FVector(X * Scale, Y * Scale, Z * Scale);
	}
};