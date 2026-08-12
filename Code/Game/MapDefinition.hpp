#pragma once
#include "ThirdParty/tinyXML2/tinyxml2.h"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"

#include <vector>
#include <string>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 typedef tinyxml2::XMLElement	XmlElement;

 class Texture;
 class Shader;
 //-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 class SpawnInfo
 {
 public:
	 SpawnInfo() = default;
	 explicit SpawnInfo(XmlElement const& spawnInfoElement);

	 ~SpawnInfo() = default;

 public:

	 std::string m_actorName;
	 Vec3		m_position = Vec3(0.f, 0.f, 0.f);
	 EulerAngles m_orientation = EulerAngles(0.f, 0.f, 0.f);
	 Vec3		m_velocity = Vec3(0.f, 0.f, 0.f);

 };

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class MapDefinition
{

public:

	MapDefinition() = default;
	explicit MapDefinition(XmlElement const& mapDefElement);

	~MapDefinition() = default;

	static void InitializeMapDefinition();
	void InitializeSpawnDefinition(XmlElement const& mapDefElement);
public:

	static std::vector<MapDefinition> s_definitions;
	
	std::vector<SpawnInfo> m_spawnDefinitions;

	std::string m_name				 = "Unknown";
	Texture*	m_spriteSheetTexture = nullptr;
	Shader*		m_mapShader			 = nullptr;

	IntVec2		m_spriteSheetCellCount;
	Image		m_mapImage;
};

