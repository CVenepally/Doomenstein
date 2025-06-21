#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/PlayerController.hpp"
#include "Game/Map.hpp"

#include "Game/MapDefinition.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/WeaponDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"

#include "Engine/Renderer/ShadowMap.hpp"

//------------------------------------------------------------------------------------------------------------------
Game* g_game = nullptr;
RandomNumberGenerator g_numGenerator;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Game::Game()
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Game::~Game()
{
	 
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Startup()
{
	PrintControlsOnDevConsole();

	m_gameClock = new Clock(Clock::GetSystemClock());

	m_screenCamera.m_viewportBounds.m_mins = Vec2::ZERO;
	m_screenCamera.m_viewportBounds.m_maxs = Vec2(static_cast<float>(g_theWindow->GetClientDimensions().x), static_cast<float>(g_theWindow->GetClientDimensions().y));

	m_timerOne = Timer(5., m_gameClock);
	m_timerTwo = Timer(0.5, m_gameClock);
	m_timerOne.Start();
	m_timerTwo.Start();
	InitializeGridVerts(25);

	AddVertsForAABB2D(m_overlayVerts, m_screenCamera.m_viewportBounds, Rgba8(0, 0, 0, 100));

	TileDefinition::InitializeTileDefinition();
	MapDefinition::InitializeMapDefinition();

	ActorDefinition::InitializeProjectileActorDefinition();
	WeaponDefinition::InitializeWeaponDefinition();
	ActorDefinition::InitializeActorDefinition();

	CreateAllSounds();
// 
	if(shouldRenderShadows)
	{
		ShadowMapConfig shadowMapConfig;
		shadowMapConfig.m_name = "Generic Shadow Map";
		shadowMapConfig.m_renderer = g_theRenderer;
		shadowMapConfig.m_shadowMapShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DepthOnly", InputLayoutType::VERTEX_P);
		shadowMapConfig.m_size = 2048;
		shadowMapConfig.m_depthBias = 0.005f;

		m_shadowMap = new ShadowMap(shadowMapConfig);

		m_shadowMap->m_camera.m_position = Vec3(0.f, 0.f, 0.5f);
		m_shadowMap->m_camera.m_orientation = EulerAngles(0.f, 60.f, 0.f);
	}
	
	SubscribeEventCallbackFunction("ShowGrid", Event_OnShowGrid);
	SubscribeEventCallbackFunction("DebugDraw", Event_DebugRender);
	SubscribeEventCallbackFunction("DebugUI", Event_DebugUI);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Shutdown()
{
	UnsubscribeEventCallbackFunction("DebugUI", Event_DebugUI);
	UnsubscribeEventCallbackFunction("DebugDraw", Event_DebugRender);
	UnsubscribeEventCallbackFunction("ShowGrid", Event_OnShowGrid);

	if(m_gameClock)
	{
		delete m_gameClock;
		m_gameClock = nullptr;
	}

	if(m_playerControllers.size() > 0)
	{
		for(PlayerController* controller : m_playerControllers)
		{
			if(controller)
			{
				delete controller;
				controller = nullptr;
			}
		}
	}

	delete m_shadowMap;
	m_shadowMap = nullptr;

	m_gridVerts.clear();

	FireEvent("Clear");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Update()
{
	PauseAndSlowmoState();

	KeyboardControls();
	XboxControls();

	AddScreenText();

	if(static_cast<int>(m_currentGameState) >= static_cast<int>(GameState::PLAYING))
	{

		for(PlayerController* playerController : m_playerControllers)
		{
			if(playerController)
			{
				playerController->Update();
			}
		}

		m_currentMap->Update();
	}

	UpdateCameras();

	for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
	{
		Vec3 position;
		Vec3 fwd;
		Vec3 up;
		Vec3 left;

		position = m_playerControllers[index]->m_position;

		m_playerControllers[index]->m_orientationDegrees.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

		g_theAudioSystem->UpdateListener(index, position, fwd, up);
	}

	if(m_currentMap && m_currentMap->m_didLevelJustStart && m_currentGameState == GameState::PLAYING)
	{
		m_currentMap->m_didLevelJustStart = false;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::AddScreenText()
{
	float fps = 1.f / m_gameClock->GetDeltaSeconds();

	float textCellHeight = 12.f;

	std::string stats = Stringf("Time: %.2f, FPS: %*.2f, TimeScale: %*.2f", m_gameClock->GetTotalSeconds(), 7, fps, 2, m_gameClock->GetTimeScale());

	AABB2 textBox;
	textBox.m_mins = Vec2::ZERO;
	textBox.m_maxs = Vec2(1590.f, 790.f);
	DebugAddScreenText(stats, textBox, textCellHeight, Vec2(1.f, 1.f), 0.f);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_theRenderer->ClearScreen(Rgba8(30, 30, 30, 255));

	switch(m_currentGameState)
	{
		case GameState::ATTRACT:
		{
			RenderAttractMode();
			break;
		}
		case GameState::LOBBY:
		{
			RenderLobby();
			break;
		}
		case GameState::LOST:
		{
			RenderLost();
			break;
		}
		case GameState::WON:
		{
			RenderWon();
			break;
		}
		case GameState::BRIEFING:
		{
			RenderBriefing();
			break;
		}
		default:
		{
			RenderGame();
			DebugRenderScreen(m_screenCamera);
			break;
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderLost()
{
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> textVerts;
	std::string attractText = "HELL WELCOMES YOU HOME";
	g_gameFont->AddVertsForTextInBox2D(textVerts, attractText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 30.f, Rgba8::RED, 0.8f, Vec2(0.5f, 0.5f), SHRINK_TO_FIT);

	std::string subText = "You died. You were not able to capture all the objectives";
	g_gameFont->AddVertsForTextInBox2D(textVerts, subText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 20.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.1f), SHRINK_TO_FIT);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderWon()
{

	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> textVerts;
	std::string attractText = "DARKNESS BOWS IN DEFEAT";
	g_gameFont->AddVertsForTextInBox2D(textVerts, attractText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 30.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.5f), SHRINK_TO_FIT);

	std::string subText = "You won! You've slain the demons. You gained control of the courtyard.";
	g_gameFont->AddVertsForTextInBox2D(textVerts, subText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 20.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.1f), SHRINK_TO_FIT);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);



}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderBriefing()
{

	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> textVerts;
	std::string attractText = "MISSION BRIEFING";
	g_gameFont->AddVertsForTextInBox2D(textVerts, attractText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 30.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.97f), SHRINK_TO_FIT);

	std::string subText = "Slayer, you are tasked to infiltrate an area sprawling with demons and capture it.\nIntel suggests that you can shutdown the demons if you can gain control of the\nfour control rooms and then the courtyard.\nYou can only capture these rooms when it's dark. You'll know when you can capture it.\nWe also got to know that new demons appear start of every day.\nDo what you must with this information...";
	g_gameFont->AddVertsForTextInBox2D(textVerts, subText, AABB2(Vec2(10.f, 5.f), Vec2(1590.f, 795.f)), 20.f, Rgba8::WHITE, 0.8f, Vec2(0.f, 0.9f), SHRINK_TO_FIT);

	std::string jump = "You can jump, btw. Hit space";
	g_gameFont->AddVertsForTextInBox2D(textVerts, jump, AABB2(Vec2(10.f, 5.f), Vec2(1590.f, 795.f)), 10.f, Rgba8::WHITE, 0.8f, Vec2(1.f, 0.f), SHRINK_TO_FIT);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);


}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::ChangeGameState(GameState gameState)
{

	m_currentGameState = gameState;

	switch(m_currentGameState)
	{
		case GameState::ATTRACT:
		{
			if(m_currentMap)
			{
				delete m_currentMap;
				m_currentMap = nullptr;
			}

			for(PlayerController* playerController : m_playerControllers)
			{
				if(playerController)
				{
					delete playerController;
					playerController = nullptr;
				}
			}

			if(m_gameClock->IsPaused())
			{
				m_gameClock->Unpause();
			}

			m_playerControllers.clear();

			break;
		}

		case GameState::LOBBY:
		{

			break;
		}

		case GameState::PLAYING:
		{
			for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
			{
				int numPlayerControllers = static_cast<int>(m_playerControllers.size());

				float viewportHeightScale = 1.f / static_cast<float>(numPlayerControllers);
				float viewportHeight = g_theWindow->GetClientDimensions().y * viewportHeightScale;

				int flippedControllerIndex = numPlayerControllers - 1 - index;

				float viewportMinY = flippedControllerIndex * viewportHeight;
				float viewportMaxY = viewportMinY + viewportHeight;

				AABB2 viewportBounds;
				viewportBounds.m_mins = Vec2(0.f, viewportMinY);
				viewportBounds.m_maxs = Vec2(static_cast<float>(g_theWindow->GetClientDimensions().x), viewportMaxY);

				m_playerControllers[index]->m_worldCamera.m_viewportBounds = viewportBounds;
				m_playerControllers[index]->m_screenCamera.m_viewportBounds = viewportBounds;
			}

			std::string defaultMap = g_gameConfigBlackboard.GetValue("defaultMap", "MPMap");
			m_currentMap = new Map(this, defaultMap);

			for(PlayerController* playerController : m_playerControllers)
			{
				if(playerController)
				{
					playerController->m_map = m_currentMap;
				}
			}
			break;
		}

		default:
			break;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameState Game::GetGameState() const
{

	return m_currentGameState;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::UpdateCameras()
{
	// #ToDo: Move all the shadow map camera things to lights or Shadow Map update Camera

	m_screenCamera.m_mode = Camera::eMode_Orthographic;
	m_screenCamera.SetOrthographicView(Vec2::ZERO, Vec2(1600.f, 800.f));

	if(shouldRenderShadows)
	{
		m_shadowMap->m_camera.m_mode = Camera::eMode_Orthographic;
		Mat44 cameraToRenderMatrix;
		cameraToRenderMatrix.SetIJKT3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 0.f));

		//	m_shadowMap->m_camera.SetCameraToRenderTransform(cameraToRenderMatrix);

		m_shadowMap->m_camera.SetOrthographicView(Vec2(0.f, 32.f), Vec2(32.f, 0.f), 0.f, 32.f);
		// 
		// 	m_worldCamera.m_mode = Camera::eMode_Perspective;

		// 	Mat44 cameraToRenderMatrix;
		// 
		// 	cameraToRenderMatrix.SetIJKT3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 0.f));
		// 
		// 	float aspect = m_shadowMap->m_camera.m_viewportBounds.m_maxs.x /m_shadowMap->m_camera.m_viewportBounds.m_maxs.y;
		// 
		// 	m_shadowMap->m_camera.SetCameraToRenderTransform(cameraToRenderMatrix);
		// 	m_shadowMap->m_camera.SetPerspectiveView(aspect, 60.f, 0.1f, 100.f);
	}

	if(static_cast<int>(m_currentGameState) >= static_cast<int>(GameState::PLAYING))
	{
		for(PlayerController* playerController : m_playerControllers)
		{
			if(playerController)
			{
				playerController->UpdateCameras();
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderAttractMode()
{
	
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> textVerts;
	std::string attractText = "Press SPACE to join with Keyboard and Mouse\nSTART or XBOX A to join with Controller\nESC, BACK or XBOX B to QUIT";

	g_gameFont->AddVertsForTextInBox2D(textVerts, attractText, AABB2(Vec2::ZERO, Vec2(1600.f, 800.f)), 24.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.05f), SHRINK_TO_FIT);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderLobby()
{
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> lobbyVerts;

	for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
	{
		int numPlayerControllers = static_cast<int>(m_playerControllers.size());

		float viewportHeightScale = 1.f / static_cast<float>(numPlayerControllers);
		float viewportHeight = 800.f * viewportHeightScale;

		int flippedControllerIndex = numPlayerControllers - 1 - index;

		float viewportMinY = flippedControllerIndex * viewportHeight;
		float viewportMaxY = viewportMinY + viewportHeight;

		AABB2 viewportBounds;
		viewportBounds.m_mins = Vec2(0.f, viewportMinY);
		viewportBounds.m_maxs = Vec2(1600.f, viewportMaxY);

		std::string playerString = Stringf("Player %d", index + 1);
		g_gameFont->AddVertsForTextInBox2D(lobbyVerts, playerString, viewportBounds, 48.f, Rgba8::WHITE, 0.8f, Vec2(0.5f, 0.5f), SHRINK_TO_FIT);

		float substringHeight = 24.f;
		Vec2 substringAlignment = Vec2(0.5f, 0.3f);

		if(m_playerControllers[index]->m_controllerID == -1)
		{
			if(m_playerControllers.size() == 1)
			{
				std::string playerSubString = Stringf("Press SPACE to start game\nPress START or XBOX A to join with controller\nPress ESC to leave lobby");
				g_gameFont->AddVertsForTextInBox2D(lobbyVerts, playerSubString, viewportBounds, substringHeight, Rgba8::WHITE, 0.8f, substringAlignment, SHRINK_TO_FIT);
			}
			else
			{
				std::string playerSubString = Stringf("Press SPACE to start game\nPress ESC to leave lobby");
				g_gameFont->AddVertsForTextInBox2D(lobbyVerts, playerSubString, viewportBounds, substringHeight, Rgba8::WHITE, 0.8f, substringAlignment, SHRINK_TO_FIT);
			}
		}
		else
		{
			if(m_playerControllers.size() == 1)
			{
				std::string playerSubString = Stringf("Press START or XBOX A to start game\nPress SPACE to add player with keyboard and mouse\nPress BACK to XBOX B to leave lobby");
				g_gameFont->AddVertsForTextInBox2D(lobbyVerts, playerSubString, viewportBounds, substringHeight, Rgba8::WHITE, 0.8f, substringAlignment, SHRINK_TO_FIT);
			}
			else
			{
				std::string playerSubString = Stringf("Press START or XBOX A to start game\nPress BACK to XBOX B to leave lobby");
				g_gameFont->AddVertsForTextInBox2D(lobbyVerts, playerSubString, viewportBounds, substringHeight, Rgba8::WHITE, 0.8f, substringAlignment, SHRINK_TO_FIT);
			}
		}

		if(m_isUIDebugMode)
		{
			AddVertsForAABB2D(lobbyVerts, viewportBounds, Rgba8(255, static_cast<uchar>(index) * 100, static_cast<uchar>(index) * 100, 100));
		}

	}

	
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(lobbyVerts);

	g_theRenderer->EndCamera(m_screenCamera);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderPlayerHUDAndWeapon(PlayerController* playerController)
{
	Actor* playerControlledActor = playerController->GetActor();

	if(playerControlledActor && playerController->m_controlMode == ControlMode::ACTOR_CONTROL)
	{
		playerControlledActor->m_currentWeapon->Render(playerController->m_screenCamera);
	}
}

//------------------------------------------------------------------------------------------------------------------
void Game::RenderGame()
{
	if(shouldRenderShadows)
	{
		m_shadowMap->UnbindAsTexture();

		g_theRenderer->BeginRenderEvent("Depth Pass");
		m_shadowMap->BeginDepthPass();
		m_currentMap->RenderDepth();
		m_shadowMap->EndDepthPass();
		g_theRenderer->EndRenderEvent("Depth Pass");

		m_shadowMap->BindAsTexture();
	}
	for(PlayerController* playerController : m_playerControllers)
	{
		if(playerController)
		{
			g_theRenderer->BeginRenderEvent("Map Render");
			g_theRenderer->BeginCamera(playerController->m_worldCamera);

			if(m_renderGrid)
			{
				RenderGrid();
			}
			m_currentMap->Render(playerController->m_worldCamera);
			DebugRenderWorld(playerController->m_worldCamera);

			g_theRenderer->EndCamera(playerController->m_worldCamera);
			g_theRenderer->EndRenderEvent("Map Render");

			g_theRenderer->BeginRenderEvent("UI Render");
			g_theRenderer->BeginCamera(playerController->m_screenCamera);

			RenderPlayerHUDAndWeapon(playerController);
			playerController->RenderHUDInfoAndDeathOverlay();

			g_theRenderer->SetModelConstants();
			g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
			g_theRenderer->BindShader(nullptr);
			g_theRenderer->BindTexture(&g_gameFont->GetTexture());
			g_theRenderer->DrawVertexArray(m_currentMap->m_textVerts);

			g_theRenderer->EndCamera(playerController->m_screenCamera);
			g_theRenderer->EndRenderEvent("UI Render");
		}
	}
	
	if(m_currentGameState == GameState::PAUSE && m_playerControllers[0]->m_controlMode == ControlMode::ACTOR_CONTROL)
	{
		g_theRenderer->BeginCamera(m_screenCamera);

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_overlayVerts);
		
		g_theRenderer->EndCamera(m_screenCamera);
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::SpawnPlayerController()
{
	for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
	{
		if(!m_playerControllers[index])
		{
			PlayerController* playerController = new PlayerController(index);
			m_playerControllers.push_back(playerController);
			return;
		}

	}

	PlayerController* playerController = new PlayerController(static_cast<int>(m_playerControllers.size()));
	m_playerControllers.push_back(playerController);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int Game::GetNumPlayerControllers()
{
	int numControllers = 0;

	for(PlayerController* playerController : m_playerControllers)
	{
		if(playerController)
		{
			numControllers += 1;
		}
	}
	return numControllers;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::InitializeGridVerts(float halfSize)
{

	float gridLineHalfLength = halfSize;
	float gridLineHalfWidth = 0.01f;
	float gridLineHalfHeight = 0.01f;

	Rgba8 xGridLinesColor = Rgba8::GREY;
	Rgba8 yGridLinesColor = Rgba8::GREY;

	AABB3 gridLineAlongX;
	gridLineAlongX.m_mins = Vec3(-gridLineHalfLength, -gridLineHalfWidth, -gridLineHalfHeight);
	gridLineAlongX.m_maxs = Vec3(gridLineHalfLength, gridLineHalfWidth, gridLineHalfHeight);

	AABB3 gridLineAlongY;
	gridLineAlongY.m_mins = Vec3(-gridLineHalfWidth, -gridLineHalfLength, -gridLineHalfHeight);
	gridLineAlongY.m_maxs = Vec3(gridLineHalfWidth, gridLineHalfLength, gridLineHalfHeight);

	for(int gridLineNum = -static_cast<int>(halfSize); gridLineNum <= static_cast<int>(halfSize); ++gridLineNum)
	{

		gridLineAlongX.SetCenter(Vec3(0.f, static_cast<float>(gridLineNum), 0.f));
		gridLineAlongY.SetCenter(Vec3(static_cast<float>(gridLineNum), 0.f, 0.f));

		if(gridLineNum == 0)
		{
			gridLineAlongX.m_mins.y = -0.05f;
			gridLineAlongX.m_maxs.y = 0.05f;

			gridLineAlongY.m_mins.x = -0.05f;
			gridLineAlongY.m_maxs.x = 0.05f;

			xGridLinesColor = Rgba8::RED;
			yGridLinesColor = Rgba8::GREEN;
		}
		else if(gridLineNum % 5 == 0)
		{

			gridLineAlongX.m_mins.y = gridLineNum - 0.02f;
			gridLineAlongX.m_maxs.y = gridLineNum + 0.02f;

			gridLineAlongY.m_mins.x = gridLineNum - 0.02f;
			gridLineAlongY.m_maxs.x = gridLineNum + 0.02f;

			xGridLinesColor = Rgba8(180, 0, 0, 255);
			yGridLinesColor = Rgba8(0, 180, 0, 255);

		}
		else
		{
			gridLineAlongX.m_mins.y = gridLineNum - 0.01f;
			gridLineAlongX.m_maxs.y = gridLineNum + 0.01f;

			gridLineAlongY.m_mins.x = gridLineNum - 0.01f;
			gridLineAlongY.m_maxs.x = gridLineNum + 0.01f;

			xGridLinesColor = Rgba8::GREY;
			yGridLinesColor = Rgba8::GREY;
		}
		AddVertsForAABB3D(m_gridVerts, gridLineAlongX, xGridLinesColor);
		AddVertsForAABB3D(m_gridVerts, gridLineAlongY, yGridLinesColor);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::PrintControlsOnDevConsole()
{
	g_devConsole->AddLine(DevConsole::INTRO_TEXT, "Doomenstein");
	g_devConsole->AddLine(DevConsole::INTRO_TEXT, "---------------------------------------------------------------------------");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Keyboard Controls");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "---------------------------------------------------------------------------");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "W / S - Move forward / move backward");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "A / D - Move left / move right");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "E / Q - Move up / move down (Fly Cam Only)");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Mouse - Look Around");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "LMB   - Attack");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Shift + any movement key - Increase movement speed");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "1 - 6 - Equip Different Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Left Arrow - Equip Previous Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Right Arrow - Equip Next Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "(For Marine) 1 - Pistol");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "(For Marine) 2 - Plasma Rifle");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Scroll Wheel UP/DOWN - Switch Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Scroll Wheel UP/DOWN - Switch Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Space - Jump");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "---------------------------------------------------------------------------");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Xbox Controls");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "---------------------------------------------------------------------------");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Left Stick - Move");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Right Stick - Look Around");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "LB / RB - Move up / move down (Fly Cam Only)");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "RT   - Attack");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "A - Increase movement speed");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Dpad Left - Equip Previous Weapon");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "Dpad Right - Equip Next Weapon");	
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "(For Marine) X - Pistol");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "(For Marine) Y - Plasma Rifle");
	g_devConsole->AddLine(DevConsole::INTRO_SUBTEXT, "---------------------------------------------------------------------------");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::RenderGrid() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_gridVerts);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::KeyboardControls()
{

	switch(m_currentGameState)
	{
		case GameState::ATTRACT:
		{
			if(g_inputSystem->WasKeyJustPressed(' '))
			{
				PlayerController* playerController = new PlayerController(-1);
				m_playerControllers.push_back(playerController);

				PlayButtonClickAudio();

				ChangeGameState(GameState::LOBBY);
			}

			if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
			{
				g_theApp->RequestQuit();
			}
			break;
		}
		case GameState::LOBBY:
		{
			bool keyboardPlayerExists = false;
			for(PlayerController* playerController : m_playerControllers)
			{
				if(playerController && playerController->m_controllerID == -1)
				{
					keyboardPlayerExists = true;
					break;
				}
			}

			if(g_inputSystem->WasKeyJustPressed(' '))
			{
				PlayButtonClickAudio();

				if(keyboardPlayerExists)
				{
					ChangeGameState(GameState::BRIEFING);
				}
				else
				{
					PlayerController* playerController = new PlayerController(-1);
					m_playerControllers.push_back(playerController);
				}
			}

			if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
			{
				if(!keyboardPlayerExists)
				{
					return;
				}
				
				PlayButtonClickAudio();

				if(m_playerControllers.size() == 1)
				{
					ChangeGameState(GameState::ATTRACT);
				}
				else
				{
					for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
					{
						if(m_playerControllers[index] && m_playerControllers[index]->m_controllerID == -1)
						{
							delete m_playerControllers[index];
							m_playerControllers.erase(m_playerControllers.begin() + index);
							keyboardPlayerExists = false;
							break;

						}
					}
				}
			}

			break;
		}
		case GameState::BRIEFING:
		{
			if(g_inputSystem->WasKeyJustPressed(' '))
			{
				PlayButtonClickAudio();

				ChangeGameState(GameState::PLAYING);

				StartGameMusic();
			}

			if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
			{
				ChangeGameState(GameState::ATTRACT);
				StartMenuMusic();
			}
			break;
		}
		default:
		{
			if(g_inputSystem->WasKeyJustPressed(KEYCODE_ESC))
			{
				ChangeGameState(GameState::ATTRACT);
				StartMenuMusic();
			}
		}
		break;
	}
	
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::XboxControls()
{
	XboxController xboxController = g_inputSystem->GetController(0);

	switch(m_currentGameState)
	{
		case GameState::ATTRACT:
		{
			if(xboxController.WasButtonJustPressed(XBOX_BUTTON_A) || xboxController.WasButtonJustPressed(XBOX_BUTTON_START))
			{
				PlayButtonClickAudio();
				PlayerController* playerController = new PlayerController(xboxController.GetControllerID());
				m_playerControllers.push_back(playerController);
				ChangeGameState(GameState::LOBBY);
			}

			if(xboxController.WasButtonJustPressed(XBOX_BUTTON_B) || xboxController.WasButtonJustPressed(XBOX_BUTTON_BACK))
			{
				g_theApp->RequestQuit();
			}
			break;
		}
		case GameState::LOBBY:
		{
			bool doesControllerPlayerExist = false;

			for(PlayerController* playerController : m_playerControllers)
			{
				if(playerController && playerController->m_controllerID == xboxController.GetControllerID())
				{
					doesControllerPlayerExist = true;
					break;
				}
			}

			if(xboxController.WasButtonJustPressed(XBOX_BUTTON_A) || xboxController.WasButtonJustPressed(XBOX_BUTTON_START))
			{
				if(doesControllerPlayerExist)
				{
					ChangeGameState(GameState::PLAYING);
					StartGameMusic();
					g_theAudioSystem->SetNumListeners(static_cast<int>(m_playerControllers.size()));
				}
				else
				{
					PlayerController* playerController = new PlayerController(0);
					m_playerControllers.push_back(playerController);
				}
				PlayButtonClickAudio();
			}

			if(xboxController.WasButtonJustPressed(XBOX_BUTTON_B) || xboxController.WasButtonJustPressed(XBOX_BUTTON_BACK))
			{
				if(!doesControllerPlayerExist)
				{
					return;
				}
				PlayButtonClickAudio();

				if(m_playerControllers.size() == 1)
				{
					ChangeGameState(GameState::ATTRACT);
				}
				else
				{
					for(int index = 0; index < static_cast<int>(m_playerControllers.size()); ++index)
					{
						if(m_playerControllers[index] && m_playerControllers[index]->m_controllerID == xboxController.GetControllerID())
						{
							delete m_playerControllers[index];

							m_playerControllers.erase(m_playerControllers.begin() + index);
							doesControllerPlayerExist = false;
							break;
						}
					}
				}
			}

			break;
		}
		default:
		{
			if(xboxController.WasButtonJustPressed(XBOX_BUTTON_BACK))
			{
				ChangeGameState(GameState::ATTRACT);
				StartMenuMusic();
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------
void Game::PauseAndSlowmoState()
{
	// add xbox support
	XboxController xboxController = g_inputSystem->GetController(0);

	if(g_inputSystem->WasKeyJustPressed('T'))
	{
		if(m_gameClock->GetTimeScale() < 1.)
		{
			m_gameClock->SetTimeScale(1.);
		}
		else
		{
			m_gameClock->SetTimeScale(0.1);
		}
	}

	if(static_cast<int>(m_currentGameState) >= static_cast<int>(GameState::PLAYING) && (g_inputSystem->WasKeyJustPressed('P') || xboxController.WasButtonJustPressed(XBOX_BUTTON_START)))
	{
		if(m_gameClock->IsPaused())
		{
			m_currentGameState = GameState::PLAYING;
		}
		else
		{
			m_currentGameState = GameState::PAUSE;
		}

		m_gameClock->TogglePause();
	}

	if(g_inputSystem->WasKeyJustPressed('O'))
	{
		
		m_gameClock->StepSingleFrame();

	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::Event_OnShowGrid(EventArgs& args)
{
	UNUSED(args);

	g_game->m_renderGrid = !g_game->m_renderGrid;

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::Event_DebugRender(EventArgs& args)
{
	UNUSED(args);
	g_game->m_isDebugMode = !g_game->m_isDebugMode;

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::Event_DebugUI(EventArgs& args)
{
	UNUSED(args);
	g_game->m_isUIDebugMode = !g_game->m_isUIDebugMode;

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Game::Event_DisableAI(EventArgs& args)
{
	UNUSED(args);
	g_game->m_aiEnabled = !g_game->m_aiEnabled;

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::CreateAllSounds()
{

	SoundID buttonClick = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("buttonClickSound", "hello"));

	if(buttonClick == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not create button click audio");
	}

	SoundID gameMusic = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("gameMusic", "hello"));

	if(gameMusic == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not create game music");
	}

	SoundID mainMenuMusic = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("mainMenuMusic", "hello"));

	if(mainMenuMusic == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not create main menu music");
	}

	m_music = g_theAudioSystem->StartSound(mainMenuMusic, true, 0.1f);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::PlayButtonClickAudio()
{
	SoundID buttonClick = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("buttonClickSound", "hello"));
	if(buttonClick == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not create button click audio");
	}
	g_theAudioSystem->StartSound(buttonClick, false, 0.3f);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::StartMenuMusic()
{
	if(m_music != MISSING_SOUND_ID)
	{
		g_theAudioSystem->StopSound(m_music);
	}

	SoundID mainMenuMusic = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("mainMenuMusic", "hello"));

	if(mainMenuMusic == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not get main menu music");
	}
	float volume = g_gameConfigBlackboard.GetValue("musicVolume", 1.f);

	m_music = g_theAudioSystem->StartSound(mainMenuMusic, true, volume);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Game::StartGameMusic()
{
	if(m_music != MISSING_SOUND_ID)
	{
		g_theAudioSystem->StopSound(m_music);
	}

	SoundID gameMusic = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard.GetValue("gameMusic", "hello"));

	if(gameMusic == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("Could not create game music");
	}

	float volume = g_gameConfigBlackboard.GetValue("musicVolume", 1.f);

	m_music = g_theAudioSystem->StartSound(gameMusic, true, volume);

}
