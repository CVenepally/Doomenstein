#pragma once

#include "Engine/Math/AABB3.hpp"
#include <string>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class TileDefinition;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Tile
{

public:

	Tile() = default;
	~Tile();

	explicit Tile(AABB3 tileBounds, std::string tileType);

	TileDefinition GetTileDefinition() const;
	AABB3 GetTileBounds() const;
	bool IsTileSolid() const;
	bool IsTileGoal() const;

private:

	AABB3   m_tileBounds;
	int		m_tileDefIndex;

};