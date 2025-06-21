#include "Game/MapDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Game/GameCommon.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<MapDefinition> MapDefinition::s_definitions;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	std::string imagePath;
	std::string textureFilePath;
	std::string shaderName;

	m_name = ParseXmlAttribute(mapDefElement, "name", m_name);
	
	imagePath = ParseXmlAttribute(mapDefElement, "image", imagePath);
	m_mapImage = Image(imagePath.c_str());
	
	textureFilePath = ParseXmlAttribute(mapDefElement, "spriteSheetTexture", textureFilePath);
	m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(textureFilePath.c_str());

	shaderName = ParseXmlAttribute(mapDefElement, "shader", shaderName);
	m_mapShader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), InputLayoutType::VERTEX_PCUTBN);
	
	m_spriteSheetCellCount = ParseXmlAttribute(mapDefElement, "spriteSheetCellCount", m_spriteSheetCellCount);

	InitializeSpawnDefinition(mapDefElement);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MapDefinition::InitializeMapDefinition()
{
	XmlDocument mapDefs;
	char const* filePath = "Data/Definitions/MapDefinitions.xml";
	XmlResult result = mapDefs.LoadFile(filePath);

	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Could not load MapDefinitions.xml");

	XmlElement* rootElement = mapDefs.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Failed to access root element of Map Definitions");

	XmlElement* mapDefElement = rootElement->FirstChildElement();

	while(mapDefElement)
	{
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", "Element name has to be \"MapDefinition\"");

		MapDefinition newMapDef = MapDefinition(*mapDefElement);
		s_definitions.push_back(newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MapDefinition::InitializeSpawnDefinition(XmlElement const& mapDefElement)
{
	// <SpawnInfos>, grand child of <Definitions>, child of <MapDefinitions>, kinda like a root (?)
	XmlElement const* spawnRootElement = mapDefElement.FirstChildElement();
	GUARANTEE_OR_DIE(spawnRootElement, "Failed to access root element of Spawn Infos");

	// <SpawnInfo>
	XmlElement const* spawnInfoElement = spawnRootElement->FirstChildElement();

	while(spawnInfoElement)
	{
		std::string elementName = spawnInfoElement->Name();
		GUARANTEE_OR_DIE(elementName == "SpawnInfo", "Element name has to be \"SpawnInfo\"");

		SpawnInfo spawnInfo = SpawnInfo(*spawnInfoElement);
		m_spawnDefinitions.push_back(spawnInfo);

		spawnInfoElement = spawnInfoElement->NextSiblingElement();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SpawnInfo::SpawnInfo(XmlElement const& spawnInfoElement)
{	
	m_actorName = ParseXmlAttribute(spawnInfoElement, "actor", m_actorName);
	m_position = ParseXmlAttribute(spawnInfoElement, "position", m_position);
	m_orientation = ParseXmlAttribute(spawnInfoElement, "orientation", m_orientation);
	m_velocity = ParseXmlAttribute(spawnInfoElement, "velocity", m_velocity);
}
