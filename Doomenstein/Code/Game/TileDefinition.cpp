#include "Game/TileDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Game/GameCommon.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<TileDefinition> TileDefinition::s_definitions;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TileDefinition::TileDefinition(XmlElement const& tileDefElement)
{
	m_name = ParseXmlAttribute(tileDefElement, "name", m_name);
	m_isSolid = ParseXmlAttribute(tileDefElement, "isSolid", m_isSolid);
	m_isGoal = ParseXmlAttribute(tileDefElement, "isGoal",  m_isGoal);
	m_mapImagePixelColor = ParseXmlAttribute(tileDefElement, "mapImagePixelColor", m_mapImagePixelColor);
	m_height = ParseXmlAttribute(tileDefElement, "tileHeight", m_height);
	m_floorSpriteCoords   = ParseXmlAttribute(tileDefElement, "floorSpriteCoords",   m_floorSpriteCoords);
	m_ceilingSpriteCoords = ParseXmlAttribute(tileDefElement, "ceilingSpriteCoords", m_ceilingSpriteCoords);
	m_wallSpriteCoords	  = ParseXmlAttribute(tileDefElement, "wallSpriteCoords",    m_wallSpriteCoords);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TileDefinition::InitializeTileDefinition()
{
	XmlDocument tileDefsXML;

	char const* filePath = "Data/Definitions/TileDefinitions.xml";
	XmlResult result = tileDefsXML.LoadFile(filePath);

	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("FAILED to open tile definitions\nFile Path: \"%s\"", filePath));

	XmlElement* rootElement = tileDefsXML.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("FAILED to access root element for tile definitions\nFile Path: \"%s\"", filePath));

	XmlElement* tileDefElement = rootElement->FirstChildElement();

	while(tileDefElement)
	{
		std::string elementName = tileDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "TileDefinition", Stringf("Root child element in %s was <%s>. Must be \"TileDefinition\"", filePath, elementName.c_str()));

		TileDefinition* newTileDef = new TileDefinition(*tileDefElement);
		s_definitions.push_back(*newTileDef);

		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string TileDefinition::GetName() const
{
	return m_name;
}
