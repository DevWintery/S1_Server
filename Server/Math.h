#pragma once

#include "Vector.h"

namespace FMath
{
	static float Square(const float A)
	{
		return A * A;
	}

	static float Sqrt(float Value)
	{
		return sqrtf(Value);
	}

	static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed)
	{
		const FVector Delta = Target - Current;
		const float DeltaM = Delta.Size();
		const float MaxStep = InterpSpeed * DeltaTime;

		if (DeltaM > MaxStep)
		{
			if (MaxStep > 0.f)
			{
				const FVector DeltaN = Delta / DeltaM;
				return Current + DeltaN * MaxStep;
			}
			else
			{
				return Current;
			}
		}

		return Target;
	}

	static float DistSquared(const FVector& V1, const FVector& V2)
	{
		return FMath::Square(V2.X - V1.X) + FMath::Square(V2.Y - V1.Y) + FMath::Square(V2.Z - V1.Z);
	}
}

