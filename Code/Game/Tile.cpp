#include "Game/Tile.hpp"
#include "Game/TileDefinition.hpp"
#include "Engine/Math/AABB3.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Tile::Tile(AABB3 tileBounds, std::string tileType)
    : m_tileBounds(tileBounds)
{
    for(int tileDefIndex = 0; tileDefIndex < static_cast<int>(TileDefinition::s_definitions.size()); ++tileDefIndex)
    {
        if(TileDefinition::s_definitions[tileDefIndex].m_name == tileType)
        {
            m_tileDefIndex = tileDefIndex;
            break;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Tile::~Tile()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TileDefinition Tile::GetTileDefinition() const
{
    return TileDefinition::s_definitions[m_tileDefIndex];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AABB3 Tile::GetTileBounds() const
{
    return m_tileBounds;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Tile::IsTileSolid() const
{
    return TileDefinition::s_definitions[m_tileDefIndex].m_isSolid;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Tile::IsTileGoal() const
{
    return TileDefinition::s_definitions[m_tileDefIndex].m_isGoal;
}
