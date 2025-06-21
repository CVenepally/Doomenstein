#pragma once

#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

//------------------------------------------------------------------------------------------------------------------
class Clock;
class PlayerController;
class Map;
class ShadowMap;

typedef size_t SoundID;
typedef size_t SoundPlaybackID;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	BRIEFING,
	PLAYING,
	LOST,
	WON,
	PAUSE,

	COUNT
};

//------------------------------------------------------------------------------------------------------------------
class Game
{

public:

	Game();
	~Game();

	void Startup();
	void Shutdown();
	void Update();
	void Render();
	void ChangeGameState(GameState gameState);
	GameState GetGameState() const;

	int	 GetNumPlayerControllers();

private:
	
	void UpdateCameras();

	void AddScreenText();
	void RenderPlayerHUDAndWeapon(PlayerController* playerController);

	void RenderAttractMode();
	void RenderLobby();
	void RenderLost();
	void RenderWon();
	void RenderBriefing();
	void RenderGame();

	void SpawnPlayerController();
	
	void InitializeGridVerts(float size);

	void PrintControlsOnDevConsole();
	void RenderGrid() const;

	void KeyboardControls();
	void XboxControls();
	
	void PauseAndSlowmoState(); 

	static bool Event_OnShowGrid(EventArgs& args);
	static bool Event_DebugRender(EventArgs& args);
	static bool Event_DebugUI(EventArgs& args);
	static bool Event_DisableAI(EventArgs& args);

	void CreateAllSounds();
	void PlayButtonClickAudio();
	void StartMenuMusic();
	void StartGameMusic();

public:

	bool m_isDebugMode = false;
	bool m_isUIDebugMode = false;
	bool m_aiEnabled = true;
	bool m_renderGrid = false;

	Clock* m_gameClock = nullptr;
	Timer  m_timerOne;
	Map* m_currentMap = nullptr;
	std::vector<PlayerController*> m_playerControllers;

	ShadowMap* m_shadowMap = nullptr;

	//temp
	bool m_isControllingPlayer = true;

private:

	GameState m_currentGameState = GameState::ATTRACT;
	Timer      m_timerTwo;
	Camera	   m_screenCamera;
	Camera	   m_testCamera;

	std::vector<Vertex_PCU> m_gridVerts;
	std::vector<Vertex_PCU> m_overlayVerts;

	SoundPlaybackID	m_music = static_cast<size_t>(-1);

	bool shouldRenderShadows = false;

};
