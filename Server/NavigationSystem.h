#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "DetourCrowd.h"

namespace DetourUtil
{
	struct NavMeshSetHeader
	{
		int magic;
		int version;
		int numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader
	{
		dtTileRef tileRef;
		int dataSize;
	};

	static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
	static const int NAVMESHSET_VERSION = 1;

	inline bool inRange(const float* v1, const float* v2, const float r, const float h)
	{
		const float dx = v2[0] - v1[0];
		const float dy = v2[1] - v1[1];
		const float dz = v2[2] - v1[2];
		return (dx * dx + dz * dz) < r * r && fabsf(dy) < h;
	}

	static bool getSteerTarget(dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
		const float minTargetDist,
		const dtPolyRef* path, const int pathSize,
		float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
		float* outPoints = 0, int* outPointCount = 0)
	{
		// Find steer target.
		static const int MAX_STEER_POINTS = 3;
		float steerPath[MAX_STEER_POINTS * 3];
		unsigned char steerPathFlags[MAX_STEER_POINTS];
		dtPolyRef steerPathPolys[MAX_STEER_POINTS];
		int nsteerPath = 0;
		navQuery->findStraightPath(startPos, endPos, path, pathSize,
			steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
		if (!nsteerPath)
			return false;

		if (outPoints && outPointCount)
		{
			*outPointCount = nsteerPath;
			for (int i = 0; i < nsteerPath; ++i)
				dtVcopy(&outPoints[i * 3], &steerPath[i * 3]);
		}


		// Find vertex far enough to steer to.
		int ns = 0;
		while (ns < nsteerPath)
		{
			// Stop at Off-Mesh link or when point is further than slop away.
			if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
				!inRange(&steerPath[ns * 3], startPos, minTargetDist, 1000.0f))
				break;
			ns++;
		}
		// Failed to find good point to steer to.
		if (ns >= nsteerPath)
			return false;

		dtVcopy(steerPos, &steerPath[ns * 3]);
		steerPos[1] = startPos[1];
		steerPosFlag = steerPathFlags[ns];
		steerPosRef = steerPathPolys[ns];

		return true;
	}

	static int dtMergeCorridorStartMoved(dtPolyRef* path, const int npath, const int maxPath,
		const dtPolyRef* visited, const int nvisited)
	{
		int furthestPath = -1;
		int furthestVisited = -1;

		// Find furthest common polygon.
		for (int i = npath - 1; i >= 0; --i)
		{
			bool found = false;
			for (int j = nvisited - 1; j >= 0; --j)
			{
				if (path[i] == visited[j])
				{
					furthestPath = i;
					furthestVisited = j;
					found = true;
				}
			}
			if (found)
				break;
		}

		// If no intersection found just return current path. 
		if (furthestPath == -1 || furthestVisited == -1)
			return npath;

		// Concatenate paths.	

		// Adjust beginning of the buffer to include the visited.
		const int req = nvisited - furthestVisited;
		const int orig = dtMin(furthestPath + 1, npath);
		int size = dtMax(0, npath - orig);
		if (req + size > maxPath)
			size = maxPath - req;
		if (size > 0)
			memmove(path + req, path + orig, size * sizeof(dtPolyRef));

		// Store visited
		for (int i = 0, n = dtMin(req, maxPath); i < n; ++i)
			path[i] = visited[(nvisited - 1) - i];

		return req + size;
	}

	static int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery* navQuery)
	{
		if (npath < 3)
			return npath;

		// Get connected polygons
		static const int maxNeis = 16;
		dtPolyRef neis[maxNeis];
		int nneis = 0;

		const dtMeshTile* tile = 0;
		const dtPoly* poly = 0;
		if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
			return npath;

		for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
		{
			const dtLink* link = &tile->links[k];
			if (link->ref != 0)
			{
				if (nneis < maxNeis)
					neis[nneis++] = link->ref;
			}
		}

		// If any of the neighbour polygons is within the next few polygons
		// in the path, short cut to that polygon directly.
		static const int maxLookAhead = 6;
		int cut = 0;
		for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
			for (int j = 0; j < nneis; j++)
			{
				if (path[i] == neis[j]) {
					cut = i;
					break;
				}
			}
		}
		if (cut > 1)
		{
			int offset = cut - 1;
			npath -= offset;
			for (int i = 1; i < npath; i++)
				path[i] = path[i + offset];
		}

		return npath;
	}

	// Returns a random number [0..1]
	static float frand()
	{
		//	return ((float)(rand() & 0xffff)/(float)0xffff);
		return (float)rand() / (float)RAND_MAX;
	}

	static dtNavMesh* DeSerializedNavMesh(const char* path)
	{
		FILE* fp = fopen(path, "rb");
		if (!fp) return 0;

		// Read header.
		NavMeshSetHeader header;
		size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
		if (readLen != 1)
		{
			fclose(fp);
			return 0;
		}
		if (header.magic != NAVMESHSET_MAGIC)
		{
			fclose(fp);
			return 0;
		}
		if (header.version != NAVMESHSET_VERSION)
		{
			fclose(fp);
			return 0;
		}

		dtNavMesh* mesh = dtAllocNavMesh();
		if (!mesh)
		{
			fclose(fp);
			return 0;
		}
		dtStatus status = mesh->init(&header.params);
		if (dtStatusFailed(status))
		{
			fclose(fp);
			return 0;
		}

		// Read tiles.
		for (int i = 0; i < header.numTiles; ++i)
		{
			NavMeshTileHeader tileHeader;
			readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
			if (readLen != 1)
			{
				fclose(fp);
				return 0;
			}

			if (!tileHeader.tileRef || !tileHeader.dataSize)
				break;

			unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
			if (!data) break;
			memset(data, 0, tileHeader.dataSize);
			readLen = fread(data, tileHeader.dataSize, 1, fp);
			if (readLen != 1)
			{
				dtFree(data);
				fclose(fp);
				return 0;
			}

			mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
		}

		fclose(fp);

		return mesh;
	}
}

class NavigationSystem
{
public:
	static NavigationSystem* GetInstance()
	{
		if (m_instance == nullptr)
		{
			m_instance = new NavigationSystem();
		}

		return m_instance;
	}

public:
	void Init(string binName);
	void ReCalculate();

	std::vector<FVector> GetRandomPath();
	std::vector<FVector> GetRandomPath(FVector start);
	std::vector<FVector> GetPaths(FVector start, FVector end);


/*Detour Crowd*/
public:
	void Update(float dt);
	int AddAgent(const FVector& position);
	void RemoveAgent(const int agentId);

	void SetRandomDestination(int agentId, float maxRadius = 2.f);
	const dtCrowdAgent* GetAgent(int agentId);

private:
	bool InitializeCrowd();
	void CleanupCrowd();

private:
	const int MAX_AGENT = 50;
	dtCrowd* m_crowd;

private:
	void SetRandomStart();
	void SetRandomEnd(float radius = 1.f);

private:
	static const int MAX_POLYS = 256;
	static const int MAX_SMOOTH = 2048;

private:
	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;
	dtQueryFilter m_filter;

	dtStatus m_pathFindStatus;
	int m_pathIterNum;
	float m_polyPickExt[3];

	float m_randomRadius;

	int m_npolys;
	dtPolyRef m_polys[MAX_POLYS];

	float m_smoothPath[MAX_SMOOTH * 3];
	int m_nsmoothPath;

	dtPolyRef m_startRef;
	float m_spos[3];
	bool m_sposSet;

	dtPolyRef m_endRef;
	float m_epos[3];
	bool m_eposSet;

private:
	static NavigationSystem* m_instance;
};