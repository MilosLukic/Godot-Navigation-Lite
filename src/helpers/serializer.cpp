#include "serializer.h"
#include <Directory.hpp>

using namespace godot;

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

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

PoolByteArray Serializer::serializeNavigationMesh(const dtNavMesh *mesh)
{
	PoolByteArray master_pba;
	if (!mesh)
		return master_pba;
	// Store header.
	NavMeshSetHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = 0;
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile *tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;
		header.numTiles++;
	}
	memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));

	PoolByteArray header_pba;
	header_pba.resize(sizeof(NavMeshSetHeader));
	{
		PoolByteArray::Write write = header_pba.write();
		memcpy(write.ptr(), &header, sizeof(NavMeshSetHeader));
	}
	master_pba.append_array(header_pba);

	// Store tiles.
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{

		const dtMeshTile *tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		PoolByteArray tile_header_pba;
		PoolByteArray tile_data_pba;
		tile_header_pba.resize(sizeof(NavMeshTileHeader));
		tile_data_pba.resize(tile->dataSize);
		{
			PoolByteArray::Write write_header = tile_header_pba.write();
			memcpy(write_header.ptr(), &tileHeader, sizeof(NavMeshTileHeader));
			PoolByteArray::Write write_data = tile_data_pba.write();
			memcpy(write_data.ptr(), tile->data, tile->dataSize);
		}
		master_pba.append_array(tile_header_pba);
		master_pba.append_array(tile_data_pba);
	}
	return master_pba;
}

dtNavMesh *Serializer::deserializeNavigationMesh(PoolByteArray byte_data)
{
	PoolByteArray::Read read_data = byte_data.read();
	NavMeshSetHeader header;
	if (byte_data.size() < sizeof(NavMeshSetHeader))
	{
		return nullptr;
	}
	memcpy(&header, read_data.ptr(), sizeof(NavMeshSetHeader));
	int seek = sizeof(NavMeshSetHeader);

	if (header.magic != NAVMESHSET_MAGIC)
	{
		return nullptr;
	}
	if (header.version != NAVMESHSET_VERSION)
	{
		return nullptr;
	}

	dtNavMesh *mesh = dtAllocNavMesh();
	if (!mesh)
	{
		return nullptr;
	}
	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		return nullptr;
	}

	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		if (byte_data.size() < seek + sizeof(tileHeader))
		{
			return nullptr;
		}
		memcpy(&tileHeader, read_data.ptr() + seek, sizeof(NavMeshTileHeader));
		seek += sizeof(NavMeshTileHeader);

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;
		unsigned char *data = (unsigned char *)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);

		if (!data)
			break;
		memset(data, 0, tileHeader.dataSize);

		if (seek + tileHeader.dataSize > byte_data.size())
		{
			return 0;
		}
		memcpy(data, read_data.ptr() + seek, tileHeader.dataSize);
		seek += tileHeader.dataSize;

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	return mesh;
}

static const int TILECACHESET_MAGIC = 'T' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'TSET';
static const int TILECACHESET_VERSION = 1;

struct TileCacheSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams meshParams;
	dtTileCacheParams cacheParams;
};

struct TileCacheTileHeader
{
	dtCompressedTileRef tileRef;
	int dataSize;
};

PoolByteArray Serializer::serializeNavigationMeshCached(const dtTileCache *m_tileCache, const dtNavMesh *m_navMesh)
{

	PoolByteArray master_pba;
	if (!m_tileCache)
		return master_pba;

	// Store header.
	TileCacheSetHeader header;
	header.magic = TILECACHESET_MAGIC;
	header.version = TILECACHESET_VERSION;
	header.numTiles = 0;
	for (int i = 0; i < m_tileCache->getTileCount(); ++i)
	{
		const dtCompressedTile *tile = m_tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;
		header.numTiles++;
	}
	memcpy(&header.cacheParams, m_tileCache->getParams(), sizeof(dtTileCacheParams));
	memcpy(&header.meshParams, m_navMesh->getParams(), sizeof(dtNavMeshParams));
	PoolByteArray header_pba;
	header_pba.resize(sizeof(TileCacheSetHeader));
	{
		PoolByteArray::Write write = header_pba.write();
		memcpy(write.ptr(), &header, sizeof(TileCacheSetHeader));
	}
	master_pba.append_array(header_pba);

	// Store tiles.
	for (int i = 0; i < m_tileCache->getTileCount(); ++i)
	{
		const dtCompressedTile *tile = m_tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		TileCacheTileHeader tileHeader;
		tileHeader.tileRef = m_tileCache->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		PoolByteArray tile_header_pba;
		PoolByteArray tile_data_pba;
		tile_header_pba.resize(sizeof(TileCacheTileHeader));
		tile_data_pba.resize(tile->dataSize);
		{
			PoolByteArray::Write write_header = tile_header_pba.write();
			memcpy(write_header.ptr(), &tileHeader, sizeof(TileCacheTileHeader));
			PoolByteArray::Write write_data = tile_data_pba.write();
			memcpy(write_data.ptr(), tile->data, tile->dataSize);
		}
		master_pba.append_array(tile_header_pba);
		master_pba.append_array(tile_data_pba);
	}
	return master_pba;
}

bool Serializer::deserializeNavigationMeshCached(PoolByteArray byte_data, dtTileCache *m_tileCache, dtNavMesh *m_navMesh, dtTileCacheMeshProcess *m_tmproc)
{
	PoolByteArray::Read read_data = byte_data.read();
	TileCacheSetHeader header;
	if (byte_data.size() < sizeof(TileCacheSetHeader))
	{
		return false;
	}
	memcpy(&header, read_data.ptr(), sizeof(TileCacheSetHeader));
	int seek = sizeof(TileCacheSetHeader);
	if (header.magic != TILECACHESET_MAGIC)
	{
		return false;
	}
	if (header.version != TILECACHESET_VERSION)
	{
		return false;
	}
	if (!m_navMesh)
	{
		return false;
	}
	dtStatus status = m_navMesh->init(&header.meshParams);
	if (dtStatusFailed(status))
	{
		return false;
	}
	if (!m_tileCache)
	{
		return false;
	}
	LinearAllocator *m_talloc = new LinearAllocator(32000);
	FastLZCompressor *m_tcomp = new FastLZCompressor;
	status = m_tileCache->init(&header.cacheParams, m_talloc, m_tcomp, m_tmproc);
	if (dtStatusFailed(status))
	{
		return false;
	}

	// validate length
	if (header.numTiles * sizeof(TileCacheSetHeader) + seek > byte_data.size())
	{
		return false;
	}
	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		TileCacheTileHeader tileHeader;
		if (sizeof(TileCacheTileHeader) + seek > byte_data.size())
		{
			return false;
		}
		memcpy(&tileHeader, read_data.ptr() + seek, sizeof(TileCacheTileHeader));
		seek += sizeof(TileCacheTileHeader);

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char *data = (unsigned char *)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data)
			break;
		memset(data, 0, tileHeader.dataSize);
		bool failure = tileHeader.dataSize + seek > byte_data.size();

		if (failure)
		{
			// Error or early EOF
			dtFree(data);
			return false;
		}
		memcpy(data, read_data.ptr() + seek, tileHeader.dataSize);
		seek += tileHeader.dataSize;
		dtCompressedTileRef tile = 0;
		dtStatus addTileStatus = m_tileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);
		if (dtStatusFailed(addTileStatus))
		{
			dtFree(data);
		}
		if (tile)
		{
			dtStatus status = m_tileCache->buildNavMeshTile(tile, m_navMesh);
		}
	}
	return true;
}