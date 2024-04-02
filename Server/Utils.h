#pragma once

#include "Vector.h"
#include <random>

class Utils
{
public:
	template<typename T>
	static T GetRandom(T min, T max)
	{
		// �õ尪�� ��� ���� random_device ����.
		std::random_device randomDevice;
		// random_device �� ���� ���� ���� ������ �ʱ�ȭ �Ѵ�.
		std::mt19937 generator(randomDevice());
		// �յ��ϰ� ��Ÿ���� �������� �����ϱ� ���� �յ� ���� ����.

		if constexpr (std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(generator);
		}
		else
		{
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(generator);
		}
	}

	static FVector RecastToUE5(float* npos)
	{
		return FVector(-npos[0], -npos[2], npos[1]);
	}

	static FVector RecastToUE5(FVector pos)
	{
		return FVector(-pos.X, -pos.Z, pos.Y);
	}

	static float* UE5ToRecast(FVector pos)
	{
		float npos[3] = { -pos.X, pos.Z, -pos.Y };
		return npos;
	}

	static FVector UE5ToRecastFVector(FVector pos)
	{
		return FVector(-pos.X, pos.Z, -pos.Y);
	}

	static FVector RecastToUE5_Meter(FVector pos)
	{
		return RecastToUE5(pos) * 100.f;
	}

	static FVector RecastToUE5_Meter(float* npos)
	{
		return RecastToUE5(npos) * 100.f;
	}

	static FVector UE5ToRecast_Meter(FVector pos)
	{
		return UE5ToRecastFVector(pos) / 100.f;
	}
};

