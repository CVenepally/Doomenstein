#pragma once
#include "ThirdParty/tinyXML2/tinyxml2.h"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <vector>
#include <string>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef tinyxml2::XMLElement	XmlElement;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Texture;
class Shader;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class TileDefinition
{
public:
	TileDefinition() = default;
	explicit TileDefinition(XmlElement const& tileDefElement);

	~TileDefinition() = default;

	static void InitializeTileDefinition();

	std::string GetName() const;

public:
	static std::vector<TileDefinition> s_definitions;

	std::string m_name;
	bool		m_isSolid;
	bool		m_isGoal = false;
	Rgba8		m_mapImagePixelColor;
	int			m_height = 1;

	IntVec2 m_floorSpriteCoords		= IntVec2(0, 0);
	IntVec2 m_ceilingSpriteCoords	= IntVec2(0, 0);
	IntVec2 m_wallSpriteCoords		= IntVec2(0, 0);
};