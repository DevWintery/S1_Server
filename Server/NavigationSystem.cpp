#include "pch.h"
#include "NavigationSystem.h"

NavigationSystem* NavigationSystem::m_instance = nullptr;

void NavigationSystem::Init(string binName)
{
	m_navMesh = DetourUtil::DeSerializedNavMesh(binName.c_str());
	if(m_navMesh == nullptr)
	{
		std::cout << "[NavgiationSystem] : Failed DeSeriaalizedNavMesh!" << std::endl;
	}
	m_navQuery = dtAllocNavMeshQuery();
	m_navQuery->init(m_navMesh, 2048);
}

void NavigationSystem::ReCalculate()
{
	if (!m_navMesh)
		return;

	if (m_sposSet)
		m_navQuery->findNearestPoly(m_spos, m_polyPickExt, &m_filter, &m_startRef, 0);
	else
		m_startRef = 0;

	if (m_eposSet)
		m_navQuery->findNearestPoly(m_epos, m_polyPickExt, &m_filter, &m_endRef, 0);
	else
		m_endRef = 0;

	m_pathFindStatus = DT_FAILURE;

	m_pathIterNum = 0;
	if (m_sposSet && m_eposSet && m_startRef && m_endRef)
	{
#ifdef DUMP_REQS
		printf("pi  %f %f %f  %f %f %f  0x%x 0x%x\n",
			m_spos[0], m_spos[1], m_spos[2], m_epos[0], m_epos[1], m_epos[2],
			m_filter.getIncludeFlags(), m_filter.getExcludeFlags());
#endif

		m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);

		m_nsmoothPath = 0;

		if (m_npolys)
		{
			// Iterate over the path to find smooth path on the detail mesh surface.
			dtPolyRef polys[MAX_POLYS];
			memcpy(polys, m_polys, sizeof(dtPolyRef) * m_npolys);
			int npolys = m_npolys;

			float iterPos[3], targetPos[3];
			m_navQuery->closestPointOnPoly(m_startRef, m_spos, iterPos, 0);
			m_navQuery->closestPointOnPoly(polys[npolys - 1], m_epos, targetPos, 0);

			static const float STEP_SIZE = 0.5f;
			static const float SLOP = 0.01f;

			m_nsmoothPath = 0;

			dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
			m_nsmoothPath++;

			// Move towards target a small advancement at a time until target reached or
			// when ran out of memory to store the path.
			while (npolys && m_nsmoothPath < MAX_SMOOTH)
			{
				// Find location to steer towards.
				float steerPos[3];
				unsigned char steerPosFlag;
				dtPolyRef steerPosRef;

				if (!DetourUtil::getSteerTarget(m_navQuery, iterPos, targetPos, SLOP,
					polys, npolys, steerPos, steerPosFlag, steerPosRef))
					break;

				bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
				bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

				// Find movement delta.
				float delta[3], len;
				dtVsub(delta, steerPos, iterPos);
				len = dtMathSqrtf(dtVdot(delta, delta));
				// If the steer target is end of path or off-mesh link, do not move past the location.
				if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
					len = 1;
				else
					len = STEP_SIZE / len;
				float moveTgt[3];
				dtVmad(moveTgt, iterPos, delta, len);

				// Move
				float result[3];
				dtPolyRef visited[16];
				int nvisited = 0;
				m_navQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter,
					result, visited, &nvisited, 16);

				npolys = DetourUtil::dtMergeCorridorStartMoved(polys, npolys, MAX_POLYS, visited, nvisited);
				npolys = DetourUtil::fixupShortcuts(polys, npolys, m_navQuery);

				float h = 0;
				m_navQuery->getPolyHeight(polys[0], result, &h);
				result[1] = h;
				dtVcopy(iterPos, result);

				// Handle end of path and off-mesh links when close enough.
				if (endOfPath && DetourUtil::inRange(iterPos, steerPos, SLOP, 1.0f))
				{
					// Reached end of path.
					dtVcopy(iterPos, targetPos);
					if (m_nsmoothPath < MAX_SMOOTH)
					{
						dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
						m_nsmoothPath++;
					}
					break;
				}
				else if (offMeshConnection && DetourUtil::inRange(iterPos, steerPos, SLOP, 1.0f))
				{
					// Reached off-mesh connection.
					float startPos[3], endPos[3];

					// Advance the path up to and over the off-mesh connection.
					dtPolyRef prevRef = 0, polyRef = polys[0];
					int npos = 0;
					while (npos < npolys && polyRef != steerPosRef)
					{
						prevRef = polyRef;
						polyRef = polys[npos];
						npos++;
					}
					for (int i = npos; i < npolys; ++i)
						polys[i - npos] = polys[i];
					npolys -= npos;

					// Handle the connection.
					dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
					if (dtStatusSucceed(status))
					{
						if (m_nsmoothPath < MAX_SMOOTH)
						{
							dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
							m_nsmoothPath++;
							// Hack to make the dotted path not visible during off-mesh connection.
							if (m_nsmoothPath & 1)
							{
								dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
								m_nsmoothPath++;
							}
						}
						// Move position at the other side of the off-mesh link.
						dtVcopy(iterPos, endPos);
						float eh = 0.0f;
						m_navQuery->getPolyHeight(polys[0], iterPos, &eh);
						iterPos[1] = eh;
					}
				}

				// Store results.
				if (m_nsmoothPath < MAX_SMOOTH)
				{
					dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
					m_nsmoothPath++;
				}
			}
		}

	}
	else
	{
		m_npolys = 0;
		m_nsmoothPath = 0;
	}
}

std::vector<FVector> NavigationSystem::GetRandomPath()
{
	SetRandomStart();
	SetRandomEnd();

	ReCalculate();

	std::vector<FVector> result;

	for (size_t i = 0; i < m_nsmoothPath * 3; i += 3)
	{
		FVector Recast(m_smoothPath[i], m_smoothPath[i + 1], m_smoothPath[i + 2]);
		result.push_back(FVector(-Recast.X * 100, -Recast.Z * 100, Recast.Y));
	}

	return result;
}

std::vector<FVector> NavigationSystem::GetRandomPath(FVector start)
{
	float startPos[3] = { -start.X / 100.f, start.Z / 100.f, -start.Y / 100.f };
	float extents[3] = { 1.0f, 1.0f, 1.0f };

	dtStatus status = m_navQuery->findNearestPoly(startPos, extents, &m_filter, &m_startRef, m_spos);
	if (dtStatusSucceed(status))
	{
		m_sposSet = true;
	}

	SetRandomEnd();

	const int maxPathSize = 1000; // Max path size
	dtPolyRef path[maxPathSize]; // Array to store the path
	int pathCount; // Store the number of polygons in the path
	m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, path, &pathCount, maxPathSize);

	float optimizedPath[maxPathSize]; // Array to store the optimized path
	int optimizedPathCount; // Store the number of polygons in the optimized path
	m_navQuery->findStraightPath(m_spos, m_epos, path, pathCount, optimizedPath, 0, 0, &optimizedPathCount, maxPathSize);

	std::vector<FVector> result;

	for (int i = 0; i < optimizedPathCount * 3; i += 3)
	{
		FVector Recast(optimizedPath[i], optimizedPath[i + 1], optimizedPath[i + 2]);
		result.push_back(FVector(-Recast.X * 100, -Recast.Z * 100, Recast.Y));
	}

	return result;
}

std::vector<FVector> NavigationSystem::GetPaths(FVector start, FVector end)
{
	std::vector<FVector> v;
	float startPos[3] = { -start.X / 100.f, start.Z / 100.f, -start.Y / 100.f };
	float endPos[3] = { -end.X / 100.f, end.Z / 100.f, -end.Y / 100.f };

	float extents[3] = { 1.0f, 1.0f, 1.0f };

	float startNearestPoint[3]; // Store the nearest point on the navmesh to the start position
	m_navQuery->findNearestPoly(startPos, extents, &m_filter, &m_startRef, startNearestPoint);

	float endNearestPoint[3]; // Store the nearest point on the navmesh to the end position
	m_navQuery->findNearestPoly(endPos, extents, &m_filter, &m_endRef, endNearestPoint);

	const int maxPathSize = 1000; // Max path size
	dtPolyRef path[maxPathSize]; // Array to store the path
	int pathCount; // Store the number of polygons in the path
	m_navQuery->findPath(m_startRef, m_endRef, startNearestPoint, endNearestPoint, &m_filter, path, &pathCount, maxPathSize);
	
	float optimizedPath[maxPathSize]; // Array to store the optimized path
	int optimizedPathCount; // Store the number of polygons in the optimized path
	m_navQuery->findStraightPath(startNearestPoint, endNearestPoint, path, pathCount, optimizedPath, 0, 0, &optimizedPathCount, maxPathSize);

	for (int i = 0; i < optimizedPathCount * 3; i += 3)
	{
		FVector Recast(optimizedPath[i], optimizedPath[i + 1], optimizedPath[i + 2]);
		v.push_back(FVector(-Recast.X * 100, -Recast.Z * 100, Recast.Y));
	}

	return v;
}

void NavigationSystem::SetRandomStart()
{
	dtStatus status = m_navQuery->findRandomPoint(&m_filter, DetourUtil::frand, &m_startRef, m_spos);
	if (dtStatusSucceed(status))
	{
		m_sposSet = true;
	}
}

void NavigationSystem::SetRandomEnd(float radius)
{
	m_randomRadius = radius;
	if (m_sposSet)
	{
		dtStatus status = m_navQuery->findRandomPointAroundCircle(m_startRef, m_spos, radius, &m_filter, DetourUtil::frand, &m_endRef, m_epos);
		if (dtStatusSucceed(status))
		{
			m_eposSet = true;
		}
	}
}
