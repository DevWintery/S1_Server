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

	InitializeCrowd();
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

void NavigationSystem::Update(float dt)
{
	if (m_crowd)
	{
		m_crowd->update(dt, nullptr);
		// Update agent positions or other state based on the crowd update.
	}
}

int NavigationSystem::AddAgent(const FVector& position)
{
	if (!m_crowd) return -1;

	/*
		radius: 에이전트의 반지름입니다. 이 값은 에이전트가 다른 에이전트나 장애물을 회피하는 데 사용됩니다.
		height: 에이전트의 높이입니다. 장애물을 넘거나 통과 가능 여부를 판단하는 데 사용됩니다.
		maxAcceleration: 에이전트의 최대 가속도입니다. 에이전트가 얼마나 빨리 가속할 수 있는지를 결정합니다.
		maxSpeed: 에이전트의 최대 속도입니다. 에이전트가 이동할 수 있는 최대 속도를 결정합니다.
		collisionQueryRange: 충돌 탐지 범위입니다. 에이전트가 주변의 다른 에이전트나 장애물을 얼마나 멀리까지 감지할 수 있는지를 결정합니다.
		pathOptimizationRange: 경로 최적화 범위입니다. 에이전트가 얼마나 멀리 떨어진 경로를 최적화할 수 있는지를 결정합니다.
		updateFlags: 에이전트의 이동 및 회피 동작을 결정하는 플래그 집합입니다. 이 플래그는 회전 예측, 시각 최적화, 위상 최적화, 장애물 회피 등을 포함할 수 있습니다.
		obstacleAvoidanceType: 장애물 회피 유형을 결정합니다. Detour Crowd는 여러 가지 장애물 회피 패턴을 제공하며, 이 값으로 사용할 패턴을 지정할 수 있습니다.
		separationWeight: 분리 가중치입니다. 에이전트가 다른 에이전트로부터 얼마나 멀리 떨어져 이동할 것인지를 결정합니다. 높은 값은 다른 에이전트와의 거리를 더 많이 유지하려는 경향을 나타냅니다.
	*/
	dtCrowdAgentParams ap;
	memset(&ap, 0, sizeof(ap));
	ap.radius = 0.6f;
	ap.height = 2.0f;
	ap.maxAcceleration = 8.0f;
	ap.maxSpeed = 10.f;
	ap.collisionQueryRange = ap.radius * 12.0f;
	ap.pathOptimizationRange = ap.radius * 30.0f;
	ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE;
	ap.obstacleAvoidanceType = 3;
	ap.separationWeight = 2.0f;

	float pos[3];
	// Convert FVector to float[3]
	pos[0] = position.X;
	pos[1] = position.Y;
	pos[2] = position.Z;

	return m_crowd->addAgent(pos, &ap);
}

void NavigationSystem::RemoveAgent(const int agentId)
{
	if (m_crowd)
	{
		m_crowd->removeAgent(agentId);
	}
}

const FVector NavigationSystem::SetRandomDestination(int agentId, float maxRadius, float speed)
{
	if (agentId < 0 || agentId >= MAX_AGENT)
	{
		return FVector::Zero();
	}

	const dtCrowdAgent* agent = m_crowd->getAgent(agentId);
	if (!agent)
	{
		return FVector::Zero();
	}

	dtPolyRef startRef;
	float nearestPt[3];
	float polyPickExt[3] = {2.0f, 4.0f, 2.0f};
	// 에이전트의 현재 위치에서 가장 가까운 폴리곤 찾기
	dtStatus status = m_navQuery->findNearestPoly(agent->npos, polyPickExt, &m_filter, &startRef, nearestPt);
	if (dtStatusFailed(status))
	{
		return FVector::Zero();
	}

	float newTarget[3];
	dtPolyRef targetRef;

	// 에이전트의 현재 위치 주변에서 랜덤한 위치 찾기
	status = m_navQuery->findRandomPointAroundCircle(startRef, agent->npos, maxRadius, &m_filter, DetourUtil::frand, &targetRef, newTarget);

	if (dtStatusSucceed(status)) 
	{
		SetAgentSpeed(agentId, speed);
		// 찾은 위치를 에이전트의 새로운 목적지로 설정
		m_crowd->requestMoveTarget(agentId, targetRef, newTarget);

		return Utils::RecastToUE5_Meter(newTarget);
	}

	return FVector::Zero();
}

void NavigationSystem::SetDestination(int agentId, const FVector& position, float speed)
{
	FVector recastPosition = Utils::UE5ToRecast_Meter(position);
	float npos[3] = { recastPosition.X, recastPosition.Y, recastPosition.Z };
	float polyPickExt[3] = { 2.0f, 4.0f, 2.0f };
	float nearest[3];
	dtPolyRef ref;
	
	dtStatus status = m_navQuery->findNearestPoly(npos, polyPickExt, &m_filter, &ref, nearest);
	if (dtStatusSucceed(status))
	{
		SetAgentSpeed(agentId, speed);
		m_crowd->requestMoveTarget(agentId, ref, nearest);
	}
}

void NavigationSystem::StopAgentMovement(int agentId)
{
	//SetAgentSpeed(agentId, 0.f);

	m_crowd->resetMoveTarget(agentId);
}

const dtCrowdAgent* NavigationSystem::GetAgent(int agentId)
{
	return m_crowd->getAgent(agentId);
}

bool NavigationSystem::IsAgentArrived(const int agentId, const float* targetPos, const float arrivalThreshold)
{
	const dtCrowdAgent* agent = m_crowd->getAgent(agentId);

	if (!agent) 
	{
		return false;
	}

	// 에이전트의 현재 위치
	const float* agentPos = agent->npos;

	// 현재 위치와 목표 위치 사이의 거리를 계산
	float distSq = dtVdistSqr(agentPos, targetPos);

	// 거리가 임계값 이하인지 확인
	return distSq <= arrivalThreshold * arrivalThreshold;
}

bool NavigationSystem::Raycast(const FVector& startPos, const FVector& endPos)
{
	FVector recast = Utils::UE5ToRecast_Meter(startPos);
	float start[3] = { recast.X, recast.Y, recast.Z };
	recast = Utils::UE5ToRecast_Meter(endPos);
	float end[3] = { recast.X, recast.Y, recast.Z };

	float t; // 레이캐스트가 장애물에 부딪힌 지점까지의 비율
	float hitNormal[3]; // 장애물에 부딪힌 지점의 노멀 벡터
	dtPolyRef path[256]; // 장애물을 찾기 위해 경로상의 폴리곤 ID를 저장하는 배열
	int pathCount; // 찾은 폴리곤의 수
	const int maxPath = 256; // path 배열의 최대 크기

	dtPolyRef startRef;
	float nearestPt[3];
	float polyPickExt[3] = { 2.0f, 4.0f, 2.0f };
	float nearest[3];
	m_navQuery->findNearestPoly(start, polyPickExt, &m_filter, &startRef, nearestPt);

	// 레이캐스트를 수행합니다.
	dtStatus status = m_navQuery->raycast(startRef, start, end, &m_filter, &t, hitNormal, path, &pathCount, maxPath);

	// t가 1보다 작다면, 레이가 장애물에 맞았음을 의미합니다.
	return t < 1.0f;
}

bool NavigationSystem::InitializeCrowd()
{
	const int maxAgents = 20; // Maximum number of agents
	const float maxAgentRadius = 0.6f; // Maximum agent radius
	m_crowd = dtAllocCrowd();
	if (!m_crowd || !m_crowd->init(maxAgents, maxAgentRadius, m_navMesh))
	{
		// Handle initialization failure
		return false;
	}

	// Additional crowd configuration as needed

	return true;
}

void NavigationSystem::SetAgentSpeed(int agentId, float speed)
{
	if (!m_crowd)
	{
		return; // DetourCrowd 객체가 초기화되지 않았다면 함수를 종료합니다.
	}

	dtCrowdAgentParams agentParams;
	const dtCrowdAgent* agent = m_crowd->getAgent(agentId);
	if (agent)
	{
		// 에이전트의 현재 파라미터를 복사합니다.
		memcpy(&agentParams, &agent->params, sizeof(dtCrowdAgentParams));
		// 최대 속도를 0으로 설정합니다.
		agentParams.maxSpeed = speed;
		// 변경된 파라미터를 적용합니다.
		m_crowd->updateAgentParameters(agentId, &agentParams);
	}
}

void NavigationSystem::CleanupCrowd()
{
	if (m_crowd)
	{
		dtFreeCrowd(m_crowd);
		m_crowd = nullptr;
	}
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
