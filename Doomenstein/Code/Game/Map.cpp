#include "Game/Map.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Tile.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerController.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/EasingFunctions.hpp"
#include "Engine/Math/CurveUtils.hpp"

extern Game* g_game;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Map::Map(Game* owner, std::string mapName)
	: m_game(owner)
{
	for(size_t mapDefIndex = 0; mapDefIndex < MapDefinition::s_definitions.size(); ++mapDefIndex)
	{
		if(MapDefinition::s_definitions[mapDefIndex].m_name == mapName)
		{
			m_mapDef = &MapDefinition::s_definitions[mapDefIndex];

			InitializeMapByImage(MapDefinition::s_definitions[mapDefIndex].m_mapImage);
			break;
		}
	}

	GenerateMapVerts();
	SpawnAllActors();

	for(int index = 0; index < static_cast<int>(m_game->m_playerControllers.size()); ++index)
	{
		SpawnPlayer();
	}

	double physicsTimeStep = static_cast<double>(1.f / 240.f);

	m_physicsTimer = Timer(physicsTimeStep, m_game->m_gameClock);
	m_physicsTimer.Start();

	float pitchDuration = g_gameConfigBlackboard.GetValue("sunPitchTimer", 1.f);
	m_sunTimer = Timer(pitchDuration, m_game->m_gameClock);
	m_sunTimer.Start();

	float yawDuration = g_gameConfigBlackboard.GetValue("sunYawTimer", 1.f);
	m_sunYawTimer = Timer(yawDuration, m_game->m_gameClock);
	m_sunYawTimer.Start();

	m_directionalLight = Light::CreateDirectionalLight(m_sunDirection);
// 	m_pointLight	   = Light::CreatePointLight(Vec3(28.f, 12.f, 0.5f), 1.f, 0.3f, 0.6f, 0.1f);

	m_didLevelJustStart = true;

	SunColors();

	g_game->m_gameClock->SetTimeScale(1.f);

	SubscribeEventCallbackFunction("KillAllActors", Event_OnKillAllActors);
	SubscribeEventCallbackFunction("SunSettings", Event_OnDisplaySunSettings);
	SubscribeEventCallbackFunction("ControlLights", Event_DebugControlLighting);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Map::~Map()
{
	UnsubscribeEventCallbackFunction("KillAllActors", Event_OnKillAllActors);
	UnsubscribeEventCallbackFunction("SunSettings", Event_OnDisplaySunSettings);
	UnsubscribeEventCallbackFunction("ControlLights", Event_DebugControlLighting);

	delete m_vbo;
	m_vbo = nullptr;

	delete m_ibo;
	m_ibo = nullptr;

	for(size_t index = 0; index < m_allActors.size(); ++index)
	{
		if(m_allActors[index])
		{
			delete m_allActors[index];
			m_allActors[index] = nullptr;
		}
	}

	for(size_t index = 0; index < m_allSpawnPoints.size(); ++index)
	{
		if(m_allSpawnPoints[index])
		{
			delete m_allSpawnPoints[index];
			m_allSpawnPoints[index] = nullptr;
		}
	}

	m_mapDef = nullptr;
	m_game = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::DisplayTime()
{
	m_textVerts.clear();

	int elapsedTime = static_cast<int>(m_sunTimer.GetElapsedTime() * 432.0);

	elapsedTime %= 86400;

	m_hours = elapsedTime / 3600;
	int minutes = (elapsedTime % 3600) / 60;
	int seconds = elapsedTime % 60;

	std::string time = Stringf("Day %d\n\n%02d:%02d:%02d", m_numDaysPassed, m_hours, minutes, seconds);

	g_gameFont->AddVertsForTextInBox2D(m_textVerts, time, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 24.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.95f), SHRINK_TO_FIT);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::InitializeMapByImage(Image& mapImage)
{	
	m_bounds = mapImage.GetDimensions();
	float tileOffset = 1.f;

	for(int y = 0; y < m_bounds.y; ++y)
	{
		for(int x = 0; x < m_bounds.x; ++x)
		{
			bool colorFound = false;

			IntVec2 texel = IntVec2(x, y);

			Rgba8 texelColor = mapImage.GetTexelColor(texel);

			if(texelColor.a == 0)
			{
				continue;
			}

			for(size_t tileDef = 0; tileDef < TileDefinition::s_definitions.size(); ++tileDef)
			{
				Rgba8 tileMapColor = TileDefinition::s_definitions[tileDef].m_mapImagePixelColor;

				if(tileMapColor.r == texelColor.r && tileMapColor.g == texelColor.g && tileMapColor.b == texelColor.b)
				{
					colorFound = true;

					std::string tileType = TileDefinition::s_definitions[tileDef].GetName();
					float tileHeight = static_cast<float>(TileDefinition::s_definitions[tileDef].m_height);

					AABB3 tileBounds;

					tileBounds.m_mins = Vec3(x , y, 0) * tileOffset;
					tileBounds.m_maxs = tileBounds.m_mins + Vec3(tileOffset, tileOffset, tileHeight);

					Tile tile(tileBounds, tileType);
					m_tiles.push_back(tile);

					if(tile.IsTileGoal())
					{
						Vec3 position = Vec3(tileBounds.m_mins.x + 0.5f, tileBounds.m_mins.y + 0.5f, 2.f);
						Light light = Light::CreateSpotLight(position, Vec3::DOWN, 20.f, 3.f, 0.33f, 0.62f, 0.05f);

						if(tileType == "BlueGoal")
						{
							light.m_color = Rgba8::BLUE.GetAsVec4();
						}
						else if(tileType == "YellowGoal")
						{
							light.m_color = Rgba8::YELLOW.GetAsVec4();
						}				
						else if(tileType == "RedGoal")
						{
							light.m_color = Rgba8::RED.GetAsVec4();

						}
						else if(tileType == "GreenGoal")
						{
							light.m_color = Rgba8::GREEN.GetAsVec4();

						}

						m_mapLights.push_back(light);
					}

					if(tileType == "BrickWall" && x != 0 && y != 0)
					{
						Vec3 position = Vec3(tileBounds.m_mins.x + 0.5f, tileBounds.m_mins.y + 0.5f, 3.f);
						Light light = Light::CreatePointLight(position, 1.f, 0.2f, 0.7f, 0.1f, Rgba8::ORANGE);
						m_mapLights.push_back(light);
					}

					break;
				}
			}

			if(!colorFound)
			{
				DebuggerPrintf(Stringf("Texel Coord: %d, %d; Color: %d %d %d\n", x, y, texelColor.r, texelColor.g, texelColor.b).c_str());
			}

		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::GenerateMapVerts()
{

	SpriteSheet mapSpriteSheet = SpriteSheet(*m_mapDef->m_spriteSheetTexture, IntVec2(8, 8));

	for(size_t tileIndex = 0; tileIndex < m_tiles.size(); ++tileIndex)
	{
		if(m_tiles[tileIndex].IsTileSolid())
		{
			AABB2 wallUVs = mapSpriteSheet.GetSpriteUVs(m_tiles[tileIndex].GetTileDefinition().m_wallSpriteCoords, 8);
			AddVertsForWall(m_verts, m_indexes, m_tiles[tileIndex].GetTileBounds(), wallUVs);
		}
		else
		{
			AABB2 floorUVs = mapSpriteSheet.GetSpriteUVs(m_tiles[tileIndex].GetTileDefinition().m_floorSpriteCoords, 8);
			AABB2 ceilingUVs = mapSpriteSheet.GetSpriteUVs(m_tiles[tileIndex].GetTileDefinition().m_ceilingSpriteCoords, 8);

//			AddVertsForCeiling(m_verts, m_indexes, m_tiles[tileIndex].GetTileBounds(), ceilingUVs);
			AddVertsForFloor(m_verts, m_indexes, m_tiles[tileIndex].GetTileBounds(), floorUVs);
//			AddVertsForFloor(verts, indexes, m_tiles[tileIndex].GetTileBounds(), floorUVs);
		}
	}

	unsigned int vboSize = sizeof(Vertex_PCUTBN) * static_cast<unsigned int>(m_verts.size());
	unsigned int iboSize = sizeof(unsigned int) * static_cast<unsigned int>(m_indexes.size());

// 	unsigned int vboSize = sizeof(Vertex_PCUTBN) * static_cast<unsigned int>(verts.size());
// 	unsigned int iboSize = sizeof(unsigned int) * static_cast<unsigned int>(indexes.size());


	m_vbo = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_ibo = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int), sizeof(unsigned int));

	g_theRenderer->CopyCPUToGPU(m_verts.data(), vboSize, m_vbo);
	g_theRenderer->CopyCPUToGPU(m_indexes.data(), iboSize, m_ibo);

//	g_theRenderer->CopyCPUToGPU(verts.data(), vboSize, m_vbo);
//	g_theRenderer->CopyCPUToGPU(indexes.data(), iboSize, m_ibo);


}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::AddVertsForWall(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	std::vector<Vec3> eightCornerPoints;
	bounds.GetCornerPoints(eightCornerPoints);

	// Positive X Face
	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_F], eightCornerPoints[POINT_E], eightCornerPoints[POINT_H], eightCornerPoints[POINT_G], Rgba8::WHITE, UVs);

	// Negative X Face
	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_D], eightCornerPoints[POINT_A], eightCornerPoints[POINT_B], eightCornerPoints[POINT_C], Rgba8::WHITE, UVs);

	// Positive Y Face
	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_E], eightCornerPoints[POINT_D], eightCornerPoints[POINT_C], eightCornerPoints[POINT_H], Rgba8::WHITE, UVs);

	// Negative Y Face
	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_A], eightCornerPoints[POINT_F], eightCornerPoints[POINT_G], eightCornerPoints[POINT_B], Rgba8::WHITE, UVs);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::AddVertsForFloor(std::vector<Vertex_PCUTBN>&verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	std::vector<Vec3> eightCornerPoints;
	bounds.GetCornerPoints(eightCornerPoints);

   	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_E], eightCornerPoints[POINT_D], eightCornerPoints[POINT_A], eightCornerPoints[POINT_F], Rgba8::WHITE, UVs);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::AddVertsForCeiling(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	std::vector<Vec3> eightCornerPoints;
	bounds.GetCornerPoints(eightCornerPoints);

	AddVertsForQuad3D(verts, indexes, eightCornerPoints[POINT_G], eightCornerPoints[POINT_B], eightCornerPoints[POINT_C], eightCornerPoints[POINT_H], Rgba8::WHITE, UVs);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::Update()
{
	m_allLights.clear();

	if(m_sunTimer.HasPeriodElapsed())
	{
		m_numDaysPassed += 1;
		RespawnDemons();
	}

	UpdateLights();

	ActorUpdate();

	if(m_physicsTimer.DecrementPeriodIfElapsed())
	{
		PhysicsUpdate();
	}

	DisplayTime();

	ActorAudioUpdate();
	ManageDeadActors();

	CheckGoalConditions();

	if(m_hours < 7 || m_hours > 20)
	{
		CheckAndFillLights();
	}

	if(m_isCourtyardCaptured)
	{
		m_game->ChangeGameState(GameState::WON);
	}
	SpawnNewPlayerIfPlayerControllerActorIsDead();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::SunColors()
{
	m_colors.push_back(Rgba8(3, 1, 20)); // 12 
	m_colors.push_back(Rgba8(9, 7, 42)); // 3
	m_colors.push_back(Rgba8(67, 94, 130)); //6
	m_colors.push_back(Rgba8(125, 186, 247)); // 9
	m_colors.push_back(Rgba8(118, 173, 227)); // 12
	m_colors.push_back(Rgba8(125, 186, 247)); // 3
	m_colors.push_back(Rgba8(254, 208, 155));  // 6
	m_colors.push_back(Rgba8(10, 10, 50)); // 9
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::UpdateLights()
{
	UpdateDirectionLights();
	UpdateSunIntensity();
	UpdateAmbientLighting();

	AABB2 textBox;
	textBox.m_mins = Vec2(5.f, 10.f);
	textBox.m_maxs = Vec2(1590.f, 790.f);
	float textCellHeight = 14.f;

	std::string manualControl;

	if(m_controlSun)
	{
		manualControl = "Manual Control: On";
	}
	else
	{
		manualControl = "Manual Control: Off";
	}

	if(m_displaySunSettings)
	{

		float pFraction = static_cast<float>(m_sunTimer.GetElapsedFraction());
		float pSeconds = static_cast<float>(m_sunTimer.GetElapsedTime());

		float yFraction = static_cast<float>(m_sunYawTimer.GetElapsedFraction());
		float ySeconds = static_cast<float>(m_sunYawTimer.GetElapsedTime());

		std::string sunDirection = Stringf("Sun Settings\n-----------------------\n%s\n[UP/DOWN]Sun Direction Pitch: %0.2f\n[LEFT/RIGHT]Sun Direction Yaw: %0.2f\nSun Direction: X: %0.2f, Y: %0.2f, Z: %0.2f, \n[F6/F7]Sun Intensity: %0.2f\n[F8/F9]Ambient Intensity: %0.2f\n\nPitch Progress: %0.2f\nElapsed Pitch Seconds: %0.2f\nYaw Progress: %0.2f\nElapsed Yaw Seconds: %0.2f",
										  manualControl.c_str(), m_sunDirectionPitch, m_sunDirectionYaw, m_sunDirection.x, m_sunDirection.y, m_sunDirection.z, m_sunIntensity, m_ambientIntensity, pFraction, pSeconds, yFraction, ySeconds);

		DebugAddScreenText(sunDirection, textBox, textCellHeight, Vec2(1.f, 0.97f), 0.f);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::UpdateDirectionLights()
{
	if(m_controlSun)
	{
		if(g_inputSystem->IsKeyDown(KEYCODE_UP_ARROW))
		{
			m_sunDirectionPitch += 0.5f;
		}

		if(g_inputSystem->IsKeyDown(KEYCODE_DOWN_ARROW))
		{
			m_sunDirectionPitch -= 0.5f;
		}

		if(g_inputSystem->IsKeyDown(KEYCODE_RIGHT_ARROW))
		{
			m_sunDirectionYaw -= 0.1f;
		}

		if(g_inputSystem->IsKeyDown(KEYCODE_LEFT_ARROW))
		{
			m_sunDirectionYaw += 0.1f;
		}

		m_sunDirection = Vec3::MakeFromPolarDegrees(m_sunDirectionPitch, m_sunDirectionYaw);
	}
	else
	{
		float t = m_sunTimer.GetElapsedFraction(); // 0.0 to 1.0 over 24h
		m_sunDirectionPitch = CosDegrees(t * 360.f) * 90.0f;
		m_sunDirectionYaw = 270.0f - (t * 180.0f);
		m_sunTimer.DecrementPeriodIfElapsed();

		m_sunDirection = Vec3::MakeFromPolarDegrees(m_sunDirectionPitch, m_sunDirectionYaw);

	}
	m_directionalLight.SetDirection(m_sunDirection);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::UpdateSunIntensity()
{
	if(m_controlSun)
	{
		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F6))
		{
			m_sunIntensity -= 0.05f;

			m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
			std::string screenMessage = Stringf("Sun Intensity: %0.2f", m_sunIntensity);

			DebugAddMessage(screenMessage, 2.f, Rgba8::ORANGE);
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F7))
		{
			m_sunIntensity += 0.05f;

			m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
			std::string screenMessage = Stringf("Sun Intensity: %0.2f", m_sunIntensity);

			DebugAddMessage(screenMessage, 2.f, Rgba8::ORANGE);
		}
	}
	
	float fraction = m_sunTimer.GetElapsedFraction();
	float scale = 1.f / 0.45f;

	if(fraction < 0.45f)
	{
		float lerpTime = fraction * scale;
		lerpTime = SmoothStart3(lerpTime);
		m_sunIntensity = Lerp(0.f, 0.9f, lerpTime);
		m_ambientIntensity = Lerp(0.05f, 0.75f, lerpTime);
	}
	else if(fraction < 0.55f)
	{
		m_sunIntensity = 0.8f;
	}
	else
	{
		float lerpTime = (fraction - 0.55f) * scale;
		lerpTime = SmoothStop6(lerpTime);
		m_sunIntensity = Lerp(0.9f, 0.f, lerpTime);
		m_ambientIntensity = Lerp(0.75f, 0.05f, lerpTime);
	}

	m_directionalLight.m_intensity = m_sunIntensity;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::UpdateAmbientLighting()
{

	if(m_controlSun)
	{
		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F8))
		{
			m_ambientIntensity -= 0.05f;

			m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
			std::string screenMessage = Stringf("Ambient Intensity: %0.2f", m_ambientIntensity);

			DebugAddMessage(screenMessage, 2.f, Rgba8::CYAN);
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_F9))
		{
			m_ambientIntensity += 0.05f;

			m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
			std::string screenMessage = Stringf("Ambient Intensity: %0.2f", m_ambientIntensity);

			DebugAddMessage(screenMessage, 2.f, Rgba8::CYAN);
		}
	}
	

// 	m_ambientIntensity = RangeMapClamped(-m_sunDirection.z, -0.8f, 1.f, 0.1f, 0.8f);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::CheckAndFillLights()
{
	for(Actor* actor : m_allActors)
	{
		if(actor && actor->m_definition->m_isLightSource)
		{
			if(IsValidPosition(actor->m_light.m_position) && static_cast<int>(m_allLights.size()) < MAX_LIGHTS)
			{
				m_allLights.push_back(actor->m_light);
			}
		}
	}

	for(Light const& light : m_mapLights)
	{
		if(IsValidPosition(light.m_position) && static_cast<int>(m_allLights.size()) < MAX_LIGHTS)
		{
			m_allLights.push_back(light);
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::IsLightInScreenSpace(Light const& light)
{
	Mat44 worldToClip = m_game->m_playerControllers[0]->m_worldCamera.GetWorldToCameraTransform();

	worldToClip.AppendTranslation3D(light.m_position);

	Vec4 clipPos = worldToClip.GetTranslation4D();

	if(clipPos.w < 0.f)
	{
		return false;
	}

	Vec3 clipDivideByW = Vec3(clipPos.x, clipPos.y, clipPos.z) / clipPos.w;

	if(clipDivideByW.x > 1.f || clipDivideByW.x < -1.f || clipDivideByW.y > 1.f || clipDivideByW.y < -1.f)
	{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::ActorUpdate()
{
	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex] && m_allActors[actorIndex]->m_state != ActorState::DEAD && m_allActors[actorIndex]->m_state != ActorState::DYING)
		{
			m_allActors[actorIndex]->Update();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::PhysicsUpdate()
{
	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex] && m_allActors[actorIndex]->m_state != ActorState::DEAD)
		{
			m_allActors[actorIndex]->PhysicsUpdate();
		}
	}

	CheckForCollisions();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::ActorAudioUpdate()
{
	for(Actor* actor : m_allActors)
	{
		if(actor)
		{
			actor->UpdateSoundPosition();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::ManageDeadActors()
{
	for(size_t index = 0; index < m_allActors.size(); ++index)
	{
		if(m_allActors[index])
		{
			m_allActors[index]->DeathStateUpdate();

			if(m_allActors[index] && m_allActors[index]->m_state == ActorState::DEAD)
			{
				delete m_allActors[index];
				m_allActors[index] = nullptr;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::SpawnNewPlayerIfPlayerControllerActorIsDead()
{
	for(PlayerController* playerController : m_game->m_playerControllers)
	{
		if(playerController->GetActor() == nullptr)
		{
			IntRange spawnPointIndexRange = IntRange(0, static_cast<int>(m_allSpawnPoints.size()) - 1);

			int randomSpawnPointIndex = spawnPointIndexRange.GetRandomInt();

			SpawnInfo marineSpawnInfo;
			marineSpawnInfo.m_actorName = "Marine";
			marineSpawnInfo.m_position = m_allSpawnPoints[randomSpawnPointIndex]->m_position;
			marineSpawnInfo.m_orientation = m_allSpawnPoints[randomSpawnPointIndex]->m_orientation;

			Actor* marineActor = SpawnActor(marineSpawnInfo);
			playerController->Possess(marineActor);
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::CheckForCollisions()
{

	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex] && m_allActors[actorIndex]->m_state != ActorState::DEAD && m_allActors[actorIndex]->m_state != ActorState::DYING)
		{
			Actor* currentActor = m_allActors[actorIndex];

			CheckActorVsActorCollision(currentActor);
			CheckActorVsMapCollision(currentActor);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::CheckActorVsActorCollision(Actor* collidingActor)
{
	Cylinder3D collisionCylinder;
	collisionCylinder.m_startPosition = collidingActor->m_position;
	collisionCylinder.m_height = collidingActor->m_definition->m_physicsHeight;
	collisionCylinder.m_radius = collidingActor->m_definition->m_physicsRadius;

	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex] && m_allActors[actorIndex]->m_definition->m_physicsSimulated)
		{
			Actor* otherActor = m_allActors[actorIndex];

			Cylinder3D otherCollisionCylinder;
			otherCollisionCylinder.m_startPosition = otherActor->m_position;
			otherCollisionCylinder.m_height = otherActor->m_definition->m_physicsHeight;
			otherCollisionCylinder.m_radius = otherActor->m_definition->m_physicsRadius;

			bool collidingSelf = otherActor == collidingActor;

			bool collidingOwner = (collidingActor->m_owner && collidingActor->m_owner == otherActor);
			bool collidingChild = (otherActor->m_owner && otherActor->m_owner == collidingActor);
			bool projectileCollision = (collidingActor->m_definition->m_name == "PlasmaProjectile") && (otherActor->m_definition->m_name == "PlasmaProjectile");

			if(collidingSelf || collidingOwner || collidingChild || projectileCollision)
			{
				continue;
			}

			if(DoCylindersOverlap(collisionCylinder, otherCollisionCylinder))
			{
				Vec2 mobileDiscCenter = Vec2(collidingActor->m_position.x, collidingActor->m_position.y);
				Vec2 fixedDiscCenter = Vec2(otherActor->m_position.x, otherActor->m_position.y);
				PushDiscsOutOfEachOther2D(mobileDiscCenter, collisionCylinder.m_radius, fixedDiscCenter, otherCollisionCylinder.m_radius);

				collidingActor->OnCollide(otherActor);

				collidingActor->m_position.x = mobileDiscCenter.x;
				collidingActor->m_position.y = mobileDiscCenter.y;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::CheckActorVsMapCollision(Actor* actor)
{
	Cylinder3D collisionCylinder;
	collisionCylinder.m_startPosition = actor->m_position;
	collisionCylinder.m_height = actor->m_definition->m_physicsHeight;
	collisionCylinder.m_radius = actor->m_definition->m_physicsRadius;

	std::vector<Tile> eightSurroundingTiles = GetEightSurroundingTiles(static_cast<int>(actor->m_position.x), static_cast<int>(actor->m_position.y));

	// wall check
	for(size_t tileIndex = 0; tileIndex < eightSurroundingTiles.size(); ++tileIndex)
	{
		if(eightSurroundingTiles[tileIndex].IsTileSolid())
		{
			if(DoesCylinderAndBoxOverlap(collisionCylinder, eightSurroundingTiles[tileIndex].GetTileBounds()))
			{
				Vec2 mobileDiscCenter = Vec2(actor->m_position.x, actor->m_position.y);
				AABB2 tileBox = AABB2(eightSurroundingTiles[tileIndex].GetTileBounds().m_mins.x, eightSurroundingTiles[tileIndex].GetTileBounds().m_mins.y, eightSurroundingTiles[tileIndex].GetTileBounds().m_maxs.x, eightSurroundingTiles[tileIndex].GetTileBounds().m_maxs.y);
				PushDiscOutOfAABB2D(mobileDiscCenter, collisionCylinder.m_radius, tileBox);

				actor->m_position.x = mobileDiscCenter.x;
				actor->m_position.y = mobileDiscCenter.y;

				actor->OnCollide();

			}
		}
		else
		{
			// ceiling and floor check
			AABB3 tileBounds = eightSurroundingTiles[tileIndex].GetTileBounds();

			Vec3 startPosition = collisionCylinder.m_startPosition;
			Vec3 endPosition = collisionCylinder.GetEndPosition();

			if(startPosition.z < tileBounds.m_mins.z)
			{
				actor->m_position.z = tileBounds.m_mins.z;
				actor->OnCollide();
				actor->m_isGrounded = true;
				actor->m_velocity.z = 0.f;
			}
			else if(startPosition.z > tileBounds.m_mins.z)
			{
				actor->m_isGrounded = false;
			}
			else
			{
				actor->m_isGrounded = true;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::Render(Camera const& camera)
{

	float scaledPercent = m_sunTimer.GetElapsedFraction() * static_cast<int>(m_colors.size());
	int startIndex = static_cast<int>(floor(scaledPercent));
	int endIndex = (startIndex + 1) % static_cast<int>(m_colors.size());

	float lerpFactor = scaledPercent - startIndex;

	Rgba8 clearColor = Rgba8::StaticColorLerp(m_colors[startIndex], m_colors[endIndex], lerpFactor);
	
	g_theRenderer->ClearScreen(clearColor);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetLightConstants(m_directionalLight, m_allLights, m_game->m_playerControllers[0]->m_position, m_ambientIntensity);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindShader(m_mapDef->m_mapShader);
	g_theRenderer->BindTexture(m_mapDef->m_spriteSheetTexture);
	g_theRenderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, static_cast<unsigned int>(m_indexes.size()));

	g_theRenderer->BeginRenderEvent("Actor Render");
	RenderAllActors(camera);
	g_theRenderer->EndRenderEvent("Actor Render");

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::RenderDepth() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
 	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, static_cast<unsigned int>(m_indexes.size()));

	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex])
		{
			m_allActors[actorIndex]->RenderDepth();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::RenderAllActors(Camera const& camera) const
{
	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex])
		{
			m_allActors[actorIndex]->Render(camera);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::RespawnDemons()
{
	for(size_t spawnIndex = 0; spawnIndex < m_mapDef->m_spawnDefinitions.size(); ++spawnIndex)
	{
		if(m_mapDef->m_spawnDefinitions[spawnIndex].m_actorName != "Marine")
		{
			SpawnActor(m_mapDef->m_spawnDefinitions[spawnIndex]);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::SpawnAllActors()
{	
	// spawn all actors in the map def
	for(size_t spawnIndex = 0; spawnIndex < m_mapDef->m_spawnDefinitions.size(); ++spawnIndex)
	{
		SpawnActor(m_mapDef->m_spawnDefinitions[spawnIndex]);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::SpawnPlayer()
{
	IntRange spawnPointIndexRange = IntRange(0, static_cast<int>(m_allSpawnPoints.size()) - 1);

	int randomSpawnPointIndex = spawnPointIndexRange.GetRandomInt();

	SpawnInfo marineSpawnInfo;
	marineSpawnInfo.m_actorName = "Marine";
	marineSpawnInfo.m_position = m_allSpawnPoints[randomSpawnPointIndex]->m_position;
	marineSpawnInfo.m_orientation = m_allSpawnPoints[randomSpawnPointIndex]->m_orientation;

	Actor* marineActor = SpawnActor(marineSpawnInfo);

	for(PlayerController* playerController : m_game->m_playerControllers)
	{
		if(playerController && (playerController->m_actorHandle == ActorHandle::INVALID))
		{
			playerController->Possess(marineActor);
			break;
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor* Map::SpawnActor(SpawnInfo const& spawnInfo)
{
	ActorHandle handle;
	
	for(size_t index = 0; index < m_allActors.size(); ++index)
	{
		if(!m_allActors[index])
		{
			handle = ActorHandle(m_currentUID, static_cast<int>(index));
			Actor* actor = new Actor(this, spawnInfo.m_actorName, spawnInfo.m_position, spawnInfo.m_orientation, handle);

			if(spawnInfo.m_actorName == "SpawnPoint")
			{
				AddActorToList(actor, m_allSpawnPoints);
				m_currentUID += 1;
				return actor;
			}

			actor->m_velocity = spawnInfo.m_velocity;

			AddActorToList(actor, m_allActors);
			m_currentUID += 1;
			return actor;
		}
	}

	handle = ActorHandle(m_currentUID, static_cast<int>(m_allActors.size()));
	Actor* actor = new Actor(this, spawnInfo.m_actorName, spawnInfo.m_position, spawnInfo.m_orientation, handle);

 	if(spawnInfo.m_actorName == "SpawnPoint")
 	{
 		AddActorToList(actor, m_allSpawnPoints);
 		m_currentUID += 1;
 		return actor;
 	}

	actor->m_velocity = spawnInfo.m_velocity;

	AddActorToList(actor, m_allActors);
	m_currentUID += 1;

	if(actor->m_definition->m_isLightSource)
	{
		m_allLights.push_back(actor->m_light);
	}

	return actor;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor* Map::GetActorByHandle(ActorHandle const& handle)
{
	if(!handle.IsValid())
	{
		return nullptr;
	}

 	int actorIndex = handle.GetIndex();

	if(!m_allActors[actorIndex])
	{
		return nullptr;
	}

	ActorHandle actorHandle = m_allActors[actorIndex]->m_handle;

	if(handle == actorHandle)
	{
		return m_allActors[actorIndex];
	}

	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::AddActorToList(Actor* actor, std::vector<Actor*>& actorList)
{
	bool isAdded = false;

	for(size_t actorIndex = 0; actorIndex < actorList.size(); ++actorIndex)
	{
		isAdded = false;

		if(actorList[actorIndex] == nullptr)
		{
			actorList[actorIndex] = actor;
			isAdded = true;
			break;
		}
	}

	if(isAdded == false)
	{
		actorList.push_back(actor);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int Map::GetTileIndexForTileCoord(IntVec2 const& tileCoord)
{
	return (tileCoord.y * m_bounds.x) + tileCoord.x;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::DoesTileExist(IntVec2 const& tileCoord)
{
	return (tileCoord.x >= 0 && tileCoord.y >= 0 && tileCoord.x < m_bounds.x && tileCoord.y < m_bounds.y);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int Map::GetActorIndexInList(Actor* actor, ActorList const& actorList)
{
	for(int index = 0; index < static_cast<int>(actorList.size()); ++index)
	{
		Actor* newActor = actorList[index];

		if(newActor && actor && (actor == newActor))
		{
			return index;
		}
	}

	return -1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::IsValidPosition(Vec3 const& position)
{
	FloatRange xRanges = FloatRange(0.f, static_cast<float>(m_bounds.x));
	FloatRange yRanges = FloatRange(0.f, static_cast<float>(m_bounds.y));
	FloatRange zRange = FloatRange(-0.0001f, 5.0001f);

	return(xRanges.IsOnRange(position.x) && yRanges.IsOnRange(position.y) && zRange.IsOnRange(position.z));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::CheckActorAreSameFaction(Actor* actorOne, Actor* actorTwo)
{
	if(actorOne && actorTwo)
	{
		return (actorOne->m_definition->m_faction == actorTwo->m_definition->m_faction);
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor* Map::GetClosestVisibleActor(Actor* searchingActor)
{

	float closestActorDistance = FLT_MAX;
	Actor* closestActor = nullptr;

	for(Actor* actor : m_allActors)
	{
		if(actor && (searchingActor != actor) && (searchingActor->m_definition->m_faction != actor->m_definition->m_faction) && (actor->m_definition->m_faction != Faction::NEUTRAL) && (actor->m_definition->m_name != "PlasmaProjectile"))
		{
			Cylinder3D actorCylinder = Cylinder3D(actor->m_position, actor->m_definition->m_physicsHeight, actor->m_definition->m_physicsRadius);

			Vec3 nearestPoint = GetNearestPointOnCylinder3D(searchingActor->GetEyePosition(), actorCylinder);

			Vec2 nearestPointXY = nearestPoint.GetXY2D();

			float sightRadius = searchingActor->m_definition->m_sightRadius;
			float sightAngle = searchingActor->m_definition->m_sightAngle;

			if(IsPointInsideDirectedSector2D(nearestPointXY, searchingActor->GetEyePosition().GetXY2D(), searchingActor->GetForwardVector().GetXY2D().GetNormalized(), sightAngle, sightRadius))
			{
				float distance = GetVectorDistanceSquared2D(nearestPointXY, searchingActor->m_position.GetXY2D());

				if(distance < closestActorDistance)
				{
					closestActorDistance = distance;

					Vec3 directionVec = (actor->m_position - searchingActor->GetEyePosition()).GetNormalized();

					RaycastResult rayVsActor = RaycastAll(searchingActor->GetEyePosition(), directionVec, sightRadius, searchingActor);

					if(rayVsActor.m_hitActor)
					{
						closestActor = actor;
					}

				}
			}
		}
	}

	return closestActor;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<Tile> Map::GetEightSurroundingTiles(int posX, int posY)
{
	std::vector<Tile> eightSurroundingTiles;

	IntVec2 frontTileCoord		= IntVec2(posX + 1, posY);
	IntVec2 backTileCoord		= IntVec2(posX - 1, posY);
	IntVec2 leftTileCoord		= IntVec2(posX, posY + 1);
	IntVec2 rightTileCoord		= IntVec2(posX, posY - 1);
	IntVec2 frontLeftTileCoord	= IntVec2(posX + 1, posY + 1);
	IntVec2 frontRightTileCoord = IntVec2(posX + 1, posY - 1);
	IntVec2 backLeftTileCoord	= IntVec2(posX - 1, posY + 1);
	IntVec2 backRightTileCoord	= IntVec2(posX - 1, posY - 1);

	if(DoesTileExist(frontTileCoord))
	{	
		int tileIndex = GetTileIndexForTileCoord(frontTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	if(DoesTileExist(backTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(backTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}
	
	if(DoesTileExist(leftTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(leftTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}
	
	if(DoesTileExist(rightTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(rightTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	if(DoesTileExist(frontLeftTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(frontLeftTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	if(DoesTileExist(frontRightTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(frontRightTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	if(DoesTileExist(backLeftTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(backLeftTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	if(DoesTileExist(backRightTileCoord))
	{
		int tileIndex = GetTileIndexForTileCoord(backRightTileCoord);
		eightSurroundingTiles.push_back(m_tiles[tileIndex]);
	}

	return eightSurroundingTiles;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::CheckGoalConditions()
{
	m_numPlayersOnGreen		= 0;
	m_numPlayersOnYellow	= 0;
	m_numPlayersOnRed		= 0;
	m_numPlayersOnBlue		= 0;

	if(m_noCaptureHourRange.IsOnRange(m_hours))
	{
		return;
	}

	for(PlayerController* player : m_game->m_playerControllers)
	{
		if(!m_isGreenCaptured &&  (m_greenXGoalRange.IsOnRange(player->m_position.x)) && (m_greenYGoalRange.IsOnRange(player->m_position.y)))
		{
			m_numPlayersOnGreen += 1;
		}

		if(!m_isYellowCaptured && (m_yellowXGoalRange.IsOnRange(player->m_position.x)) && (m_yellowYGoalRange.IsOnRange(player->m_position.y)))
		{
			m_numPlayersOnYellow += 1;
		}

		if(!m_isBlueCaptured && (m_blueXGoalRange.IsOnRange(player->m_position.x)) && (m_blueYGoalRange.IsOnRange(player->m_position.y)))
		{
			m_numPlayersOnBlue += 1;
		}

		if(!m_isRedCaptured && (m_redXGoalRange.IsOnRange(player->m_position.x)) && (m_redYGoalRange.IsOnRange(player->m_position.y)))
		{
			m_numPlayersOnRed += 1;
		}
	}

	int captureRate = 10;

	if(m_numPlayersOnGreen > 0)
	{
		m_greenCapturePercent += m_game->m_gameClock->GetDeltaSeconds() * captureRate;

		std::string capture = Stringf("Capturing Green : %0.2f", m_greenCapturePercent);

		g_gameFont->AddVertsForTextInBox2D(m_textVerts, capture, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 18.f, Rgba8::GREEN, 1.f, Vec2(0.01f, 0.98f), SHRINK_TO_FIT);

	}
	else if(!m_isGreenCaptured)
	{
		m_greenCapturePercent -= m_game->m_gameClock->GetDeltaSeconds() * captureRate;
	}
	
	if(m_numPlayersOnYellow > 0)
	{
		m_yellowCapturePercent += m_game->m_gameClock->GetDeltaSeconds() * captureRate;

		std::string capture = Stringf("Capturing Yellow : %0.2f", m_yellowCapturePercent);

		g_gameFont->AddVertsForTextInBox2D(m_textVerts, capture, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 18.f, Rgba8::GREEN, 1.f, Vec2(0.01f, 0.98f), SHRINK_TO_FIT);

	}
	else if(!m_isYellowCaptured)
	{
		m_yellowCapturePercent -= m_game->m_gameClock->GetDeltaSeconds() * captureRate;
	}

	if(m_numPlayersOnRed > 0)
	{
		m_redCapturePercent += m_game->m_gameClock->GetDeltaSeconds() * captureRate;

		std::string capture = Stringf("Capturing Red : %0.2f", m_redCapturePercent);

		g_gameFont->AddVertsForTextInBox2D(m_textVerts, capture, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 18.f, Rgba8::GREEN, 1.f, Vec2(0.01f, 0.98f), SHRINK_TO_FIT);

	}
	else if(!m_isRedCaptured)
	{
		m_redCapturePercent -= m_game->m_gameClock->GetDeltaSeconds() * captureRate;

	}
	
	if(m_numPlayersOnBlue > 0)
	{
		m_blueCapturePercent += m_game->m_gameClock->GetDeltaSeconds() * captureRate;

		std::string capture = Stringf("Capturing Blue : %0.2f", m_blueCapturePercent);

		g_gameFont->AddVertsForTextInBox2D(m_textVerts, capture, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 18.f, Rgba8::GREEN, 1.f, Vec2(0.01f, 0.98f), SHRINK_TO_FIT);

	}
	else if(!m_isBlueCaptured)
	{
		m_blueCapturePercent -= m_game->m_gameClock->GetDeltaSeconds() * captureRate;
	}

	m_greenCapturePercent	= GetClamped(m_greenCapturePercent, 0.f, 100.f);
	m_yellowCapturePercent	= GetClamped(m_yellowCapturePercent, 0.f, 100.f);
	m_blueCapturePercent	= GetClamped(m_blueCapturePercent, 0.f, 100.f);
	m_redCapturePercent		= GetClamped(m_redCapturePercent, 0.f, 100.f);

	if(!m_isGreenCaptured && m_greenCapturePercent >= 100.f)
	{
		m_isGreenCaptured = true;

		for(Light& light : m_mapLights)
		{
			if(light.m_lightType == static_cast<int>(LightType::SPOT) && light.m_color == Rgba8::GREEN.GetAsVec4())
			{
				light.m_intensity = 0.f;
			}
		}

		m_numZonesCaptured += 1;
	}
	
	if(!m_isYellowCaptured && m_yellowCapturePercent >= 100.f)
	{
		m_isYellowCaptured = true;
		for(Light& light : m_mapLights)
		{
			if(light.m_lightType == static_cast<int>(LightType::SPOT) && light.m_color == Rgba8::YELLOW.GetAsVec4())
			{
				light.m_intensity = 0.f;
			}
		}
		m_numZonesCaptured += 1;
	}

	if(!m_isBlueCaptured && m_blueCapturePercent >= 100.f)
	{
		m_isBlueCaptured = true;

		for(Light& light : m_mapLights)
		{
			if(light.m_lightType == static_cast<int>(LightType::SPOT) && light.m_color == Rgba8::BLUE.GetAsVec4())
			{
				light.m_intensity = 0.f;
			}
		}
		
		m_numZonesCaptured += 1;
	}

	if(!m_isRedCaptured && m_redCapturePercent >= 100.f)
	{
		m_isRedCaptured = true;

		for(Light& light : m_mapLights)
		{
			if(light.m_lightType == static_cast<int>(LightType::SPOT) && light.m_color == Rgba8::RED.GetAsVec4())
			{
				light.m_intensity = 0.f;
			}
		}

		m_numZonesCaptured += 1;
	}

	if(m_numZonesCaptured >= 4)
	{
		for(PlayerController* player : m_game->m_playerControllers)
		{
			if(!m_isCourtyardCaptured && (m_courtyardXGoalRange.IsOnRange(player->m_position.x)) && (m_courtyardYGoalRange.IsOnRange(player->m_position.y)))
			{
				m_numPlayersOnCourtyard += 1;
			}
		}

		if(m_numPlayersOnCourtyard > 0)
		{
			m_courtyardCapturePercent += m_game->m_gameClock->GetDeltaSeconds() * captureRate * 0.3f;

			std::string capture = Stringf("Capturing Courtyard : %0.2f", m_courtyardCapturePercent);

			g_gameFont->AddVertsForTextInBox2D(m_textVerts, capture, AABB2(Vec2::ZERO, g_theWindow->GetClientDimensions().GetAsVec2()), 18.f, Rgba8::GREEN, 1.f, Vec2(0.01f, 0.98f), SHRINK_TO_FIT);

		}
		else if(!m_isCourtyardCaptured)
		{
			m_courtyardCapturePercent -= m_game->m_gameClock->GetDeltaSeconds() * captureRate * 0.3f;
		}

		m_courtyardCapturePercent = GetClamped(m_courtyardCapturePercent, 0.f, 100.f);

		if(!m_isCourtyardCaptured && m_courtyardCapturePercent >= 100.f)
		{
			m_isCourtyardCaptured = true;
			m_numZonesCaptured += 1;
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult Map::RaycastAll(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance, Actor* firingActor)
{

	RaycastResult closestRaycast;
	RaycastResult raycastVsActors  = RaycastVsActors(startPosition, fwdNormal, distance, firingActor);
	RaycastResult raycastVsFloor   = RaycastVsFloor(startPosition, fwdNormal, distance);
	RaycastResult raycastVsWalls   = RaycastVsWalls(startPosition, fwdNormal, distance);

	if(raycastVsActors.m_rayResult.m_impactDistance < raycastVsFloor.m_rayResult.m_impactDistance && raycastVsActors.m_rayResult.m_impactDistance < raycastVsWalls.m_rayResult.m_impactDistance)
	{
		closestRaycast = raycastVsActors;
	} 
	else if(raycastVsFloor.m_rayResult.m_impactDistance < raycastVsWalls.m_rayResult.m_impactDistance)
	{
		closestRaycast = raycastVsFloor;
	}
	else
	{
		closestRaycast = raycastVsWalls;
	}

	return closestRaycast;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult Map::RaycastVsActors(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance, Actor* firingActor)
{
	RaycastResult closestRaycast;
	closestRaycast.m_rayResult.m_impactDistance = 9999999.f;
	closestRaycast.m_rayResult.m_rayStartPos = startPosition;
	closestRaycast.m_rayResult.m_rayForwardNormal = fwdNormal;
	closestRaycast.m_rayResult.m_rayMaxLength = distance;

	for(size_t actorIndex = 0; actorIndex < m_allActors.size(); ++actorIndex)
	{
		if(m_allActors[actorIndex])
		{
			if(m_allActors[actorIndex] == firingActor)
			{
				continue;
			}

			Cylinder3D cylinder = Cylinder3D(m_allActors[actorIndex]->m_position, m_allActors[actorIndex]->m_definition->m_physicsHeight, m_allActors[actorIndex]->m_definition->m_physicsRadius);

			RaycastResult3D rayVsActor = RaycastVsCylinder3D(startPosition, fwdNormal, distance, cylinder);

			if(rayVsActor.m_didImpact && rayVsActor.m_impactDistance < closestRaycast.m_rayResult.m_impactDistance)
			{
				closestRaycast.m_rayResult = rayVsActor;
				closestRaycast.m_hitActor = m_allActors[actorIndex];
			}
		}
	}

	return closestRaycast;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult Map::RaycastVsCeiling(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance)
{
	RaycastResult raycastResult;
	raycastResult.m_rayResult.m_didImpact = false;
	raycastResult.m_rayResult.m_impactDistance = 9999999.f;
	raycastResult.m_rayResult.m_rayStartPos = startPosition;
	raycastResult.m_rayResult.m_rayForwardNormal = fwdNormal;
	raycastResult.m_rayResult.m_rayMaxLength = distance;
	raycastResult.m_hitActor = nullptr;

	Vec3 endPosition = startPosition + fwdNormal * distance;

	Vec3 rayVector = endPosition - startPosition;

	float t = (1.f - startPosition.z) / rayVector.z;
	
	FloatRange overlapRange = FloatRange(0.f, 1.f);

	if(!overlapRange.IsOnRange(t))
	{
		raycastResult.m_rayResult.m_didImpact = false;
		return raycastResult;
	}
	
	raycastResult.m_rayResult.m_impactDistance = distance * t;
	raycastResult.m_rayResult.m_impactPos = startPosition + (fwdNormal * raycastResult.m_rayResult.m_impactDistance);

	if(!IsValidPosition(raycastResult.m_rayResult.m_impactPos))
	{
		raycastResult.m_rayResult.m_didImpact = false;
		return raycastResult;
	}

	raycastResult.m_rayResult.m_didImpact = true;
	raycastResult.m_rayResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);

	return raycastResult;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult Map::RaycastVsFloor(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance)
{
	RaycastResult raycastResult;
	raycastResult.m_rayResult.m_didImpact = false;
	raycastResult.m_rayResult.m_impactDistance = 9999999.f;
	raycastResult.m_rayResult.m_rayStartPos = startPosition;
	raycastResult.m_rayResult.m_rayForwardNormal = fwdNormal;
	raycastResult.m_rayResult.m_rayMaxLength = distance;
	raycastResult.m_hitActor = nullptr;

	Vec3 rayVector = fwdNormal * distance;

	float t = (-startPosition.z) / rayVector.z;

	FloatRange overlapRange = FloatRange(0.f, 1.f);

	if(!overlapRange.IsOnRange(t))
	{
		raycastResult.m_rayResult.m_didImpact = false;
		return raycastResult;
	}

	raycastResult.m_rayResult.m_impactDistance = distance * t;
	raycastResult.m_rayResult.m_impactPos = startPosition + (fwdNormal * raycastResult.m_rayResult.m_impactDistance);

	if(!IsValidPosition(raycastResult.m_rayResult.m_impactPos))
	{
		raycastResult.m_rayResult.m_didImpact = false;
		return raycastResult;
	}

	raycastResult.m_rayResult.m_didImpact = true;
	raycastResult.m_rayResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);

	return raycastResult;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RaycastResult Map::RaycastVsWalls(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance)
{
	RaycastResult raycastResult;
	raycastResult.m_rayResult.m_didImpact = false;
	raycastResult.m_rayResult.m_impactDistance = 9999999.f;
	raycastResult.m_rayResult.m_rayStartPos = startPosition;
	raycastResult.m_rayResult.m_rayForwardNormal = fwdNormal;
	raycastResult.m_rayResult.m_rayMaxLength = distance;
	raycastResult.m_hitActor = nullptr;

	FloatRange zRange = FloatRange(0.f, 1.f);
	IntRange  tileIndexRange = IntRange(0, static_cast<int>(m_tiles.size()) - 1);

	IntVec2 startTileCoord = IntVec2(static_cast<int>(startPosition.x), static_cast<int>(startPosition.y));
	int startTileIndex = GetTileIndexForTileCoord(startTileCoord);

	if(tileIndexRange.IsOnRange(startTileIndex)) 
	{
		if(m_tiles[startTileIndex].IsTileSolid() && zRange.IsOnRange(startPosition.z))
		{
			raycastResult.m_rayResult.m_didImpact = true;
			raycastResult.m_rayResult.m_impactDistance = 0.f;
			raycastResult.m_rayResult.m_impactPos = startPosition;
			raycastResult.m_rayResult.m_impactNormal = -fwdNormal;

			return raycastResult;
	    }
	}

	// step directions
	int xStepDirection = fwdNormal.x < 0.f ? -1 : 1;
	int yStepDirection = fwdNormal.y < 0.f ? -1 : 1;

	// x
	float fwdDistancePerX =	1.f / abs(fwdNormal.x);
	float xPosAtFirstXCrossing = static_cast<float>(startTileCoord.x + (xStepDirection + 1) / 2);
	float xDistToFirstXCrossing = xPosAtFirstXCrossing - startPosition.x;
	float fwdDistanceAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistancePerX;

	// y
	float fwdDistancePerY = 1.f / abs(fwdNormal.y);
	float yPosAtFirstYCrossing = static_cast<float>(startTileCoord.y + (yStepDirection + 1) / 2);
	float yDistToFirstYCrossing = yPosAtFirstYCrossing - startPosition.y;
	float fwdDistanceAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistancePerY;

	while(true)
	{
		if(fwdDistanceAtNextXCrossing < fwdDistanceAtNextYCrossing)
		{
			if(fwdDistanceAtNextXCrossing > distance)
			{
				raycastResult.m_rayResult.m_didImpact = false;
				raycastResult.m_rayResult.m_impactDistance = distance;
				return raycastResult;
			}

			startTileCoord.x += xStepDirection;
			
			int tileIndex = GetTileIndexForTileCoord(startTileCoord);

			if(tileIndex > 0 && tileIndex < static_cast<int>(m_tiles.size()) && m_tiles[tileIndex].IsTileSolid())
			{
				raycastResult.m_rayResult.m_impactDistance = fwdDistanceAtNextXCrossing;
				raycastResult.m_rayResult.m_impactPos = startPosition + (raycastResult.m_rayResult.m_impactDistance * raycastResult.m_rayResult.m_rayForwardNormal);

			if(IsValidPosition(raycastResult.m_rayResult.m_impactPos))
				{
					raycastResult.m_rayResult.m_impactNormal = Vec3(-xStepDirection, 0, 0);
					raycastResult.m_rayResult.m_didImpact = true;
					break;
				}
			}

			fwdDistanceAtNextXCrossing += fwdDistancePerX;

		}
		else
		{
			if(fwdDistanceAtNextYCrossing > distance)
			{
				raycastResult.m_rayResult.m_didImpact = false;
				raycastResult.m_rayResult.m_impactDistance = distance;
				return raycastResult;
			}

			startTileCoord.y += yStepDirection;

			int tileIndex = GetTileIndexForTileCoord(startTileCoord);

			if(tileIndex > 0 && tileIndex < static_cast<int>(m_tiles.size()) && m_tiles[tileIndex].IsTileSolid())
			{
				raycastResult.m_rayResult.m_impactDistance = fwdDistanceAtNextYCrossing;
				raycastResult.m_rayResult.m_impactPos = startPosition + (raycastResult.m_rayResult.m_impactDistance * raycastResult.m_rayResult.m_rayForwardNormal);

				if(IsValidPosition(raycastResult.m_rayResult.m_impactPos))
				{
					raycastResult.m_rayResult.m_impactNormal = Vec3(0, -yStepDirection, 0);
					raycastResult.m_rayResult.m_didImpact = true;
					break;
				}
			}

			fwdDistanceAtNextYCrossing += fwdDistancePerY;
		}
	}

	return raycastResult;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Map::DebugPossessNext()
{

	if(static_cast<int>(m_game->m_playerControllers.size() > 1))
	{
		return;
	}

	PlayerController* playerController = m_game->m_playerControllers[0];

	Actor* possessedActor = playerController->GetActor();

	int possessedActorIndex = GetActorIndexInList(possessedActor, m_allActors);

	if(possessedActorIndex >= 0)
	{
		int newActorIndex = (possessedActorIndex + 1) % m_allActors.size();
		Actor* newActor = m_allActors[newActorIndex];

		while(!newActor || (newActor && !newActor->m_definition->m_canBePossessed))
		{
			newActorIndex = (newActorIndex + 1) % m_allActors.size();
			newActor = m_allActors[newActorIndex];
		}

		playerController->Possess(newActor);

	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::Event_OnKillAllActors(EventArgs& args)
{
	UNUSED(args);

	for(int index = 0; index < static_cast<int>(g_game->m_currentMap->m_allActors.size()); ++index)
	{
		if(g_game->m_currentMap->m_allActors[index] && g_game->m_currentMap->m_allActors[index]->m_definition->m_canBePossessed)
		{
			g_game->m_currentMap->m_allActors[index]->m_health = 0;
			g_game->m_currentMap->m_allActors[index]->SetActorState(ActorState::DYING);
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::Event_OnDisplaySunSettings(EventArgs& args)
{
	UNUSED(args);

	g_game->m_currentMap->m_displaySunSettings = !g_game->m_currentMap->m_displaySunSettings;

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Map::Event_DebugControlLighting(EventArgs& args)
{ 
	UNUSED(args);

	g_game->m_currentMap->m_controlSun = !g_game->m_currentMap->m_controlSun;

	return false;
}
