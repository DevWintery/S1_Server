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
		radius: ������Ʈ�� �������Դϴ�. �� ���� ������Ʈ�� �ٸ� ������Ʈ�� ��ֹ��� ȸ���ϴ� �� ���˴ϴ�.
		height: ������Ʈ�� �����Դϴ�. ��ֹ��� �Ѱų� ��� ���� ���θ� �Ǵ��ϴ� �� ���˴ϴ�.
		maxAcceleration: ������Ʈ�� �ִ� ���ӵ��Դϴ�. ������Ʈ�� �󸶳� ���� ������ �� �ִ����� �����մϴ�.
		maxSpeed: ������Ʈ�� �ִ� �ӵ��Դϴ�. ������Ʈ�� �̵��� �� �ִ� �ִ� �ӵ��� �����մϴ�.
		collisionQueryRange: �浹 Ž�� �����Դϴ�. ������Ʈ�� �ֺ��� �ٸ� ������Ʈ�� ��ֹ��� �󸶳� �ָ����� ������ �� �ִ����� �����մϴ�.
		pathOptimizationRange: ��� ����ȭ �����Դϴ�. ������Ʈ�� �󸶳� �ָ� ������ ��θ� ����ȭ�� �� �ִ����� �����մϴ�.
		updateFlags: ������Ʈ�� �̵� �� ȸ�� ������ �����ϴ� �÷��� �����Դϴ�. �� �÷��״� ȸ�� ����, �ð� ����ȭ, ���� ����ȭ, ��ֹ� ȸ�� ���� ������ �� �ֽ��ϴ�.
		obstacleAvoidanceType: ��ֹ� ȸ�� ������ �����մϴ�. Detour Crowd�� ���� ���� ��ֹ� ȸ�� ������ �����ϸ�, �� ������ ����� ������ ������ �� �ֽ��ϴ�.
		separationWeight: �и� ����ġ�Դϴ�. ������Ʈ�� �ٸ� ������Ʈ�κ��� �󸶳� �ָ� ������ �̵��� �������� �����մϴ�. ���� ���� �ٸ� ������Ʈ���� �Ÿ��� �� ���� �����Ϸ��� ������ ��Ÿ���ϴ�.
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
	// ������Ʈ�� ���� ��ġ���� ���� ����� ������ ã��
	dtStatus status = m_navQuery->findNearestPoly(agent->npos, polyPickExt, &m_filter, &startRef, nearestPt);
	if (dtStatusFailed(status))
	{
		return FVector::Zero();
	}

	float newTarget[3];
	dtPolyRef targetRef;

	// ������Ʈ�� ���� ��ġ �ֺ����� ������ ��ġ ã��
	status = m_navQuery->findRandomPointAroundCircle(startRef, agent->npos, maxRadius, &m_filter, DetourUtil::frand, &targetRef, newTarget);

	if (dtStatusSucceed(status)) 
	{
		SetAgentSpeed(agentId, speed);
		// ã�� ��ġ�� ������Ʈ�� ���ο� �������� ����
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

	// ������Ʈ�� ���� ��ġ
	const float* agentPos = agent->npos;

	// ���� ��ġ�� ��ǥ ��ġ ������ �Ÿ��� ���
	float distSq = dtVdistSqr(agentPos, targetPos);

	// �Ÿ��� �Ӱ谪 �������� Ȯ��
	return distSq <= arrivalThreshold * arrivalThreshold;
}

bool NavigationSystem::Raycast(const FVector& startPos, const FVector& endPos)
{
	FVector recast = Utils::UE5ToRecast_Meter(startPos);
	float start[3] = { recast.X, recast.Y, recast.Z };
	recast = Utils::UE5ToRecast_Meter(endPos);
	float end[3] = { recast.X, recast.Y, recast.Z };

	float t; // ����ĳ��Ʈ�� ��ֹ��� �ε��� ���������� ����
	float hitNormal[3]; // ��ֹ��� �ε��� ������ ��� ����
	dtPolyRef path[256]; // ��ֹ��� ã�� ���� ��λ��� ������ ID�� �����ϴ� �迭
	int pathCount; // ã�� �������� ��
	const int maxPath = 256; // path �迭�� �ִ� ũ��

	dtPolyRef startRef;
	float nearestPt[3];
	float polyPickExt[3] = { 2.0f, 4.0f, 2.0f };
	float nearest[3];
	m_navQuery->findNearestPoly(start, polyPickExt, &m_filter, &startRef, nearestPt);

	// ����ĳ��Ʈ�� �����մϴ�.
	dtStatus status = m_navQuery->raycast(startRef, start, end, &m_filter, &t, hitNormal, path, &pathCount, maxPath);

	// t�� 1���� �۴ٸ�, ���̰� ��ֹ��� �¾����� �ǹ��մϴ�.
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
		return; // DetourCrowd ��ü�� �ʱ�ȭ���� �ʾҴٸ� �Լ��� �����մϴ�.
	}

	dtCrowdAgentParams agentParams;
	const dtCrowdAgent* agent = m_crowd->getAgent(agentId);
	if (agent)
	{
		// ������Ʈ�� ���� �Ķ���͸� �����մϴ�.
		memcpy(&agentParams, &agent->params, sizeof(dtCrowdAgentParams));
		// �ִ� �ӵ��� 0���� �����մϴ�.
		agentParams.maxSpeed = speed;
		// ����� �Ķ���͸� �����մϴ�.
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
