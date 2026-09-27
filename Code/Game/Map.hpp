#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Renderer/Light.hpp"

#include "Game/Tile.hpp"

#include <string>
#include <vector>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class	Game;
class	Image;
class	Texture;
class	Shader;
class	VertexBuffer;
class	IndexBuffer;
class	MapDefinition;
class	Actor;
class	Camera;
class	SpawnInfo;
class	PlayerController;
class	NamedStrings;

struct	LightConstants;
struct	RaycastResult3D;
struct	AABB2;
struct	RaycastResult;
struct	ActorHandle;

typedef std::vector<Actor*> ActorList;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Objective banner shown under the clock for OBJECTIVE_MESSAGE_DURATION seconds.
struct TimedStatusMessage
{
	std::string m_text;
	Rgba8		m_color = Rgba8::WHITE;
	bool		m_isFlashing = false;
	Timer		m_timer;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Map
{
public:

	Map(Game* owner, std::string mapName);
	~Map();

	void				Update();
	void				Render(Camera const& camera);
	void				RenderDepth() const;
						
	Actor*				SpawnActor(SpawnInfo const& spawnInfo);
	Actor*				GetActorByHandle(ActorHandle const& handle);
						
	int					GetTileIndexForTileCoord(IntVec2 const& tileCoord);
	bool				DoesTileExist(IntVec2 const& tileCoord);
	bool				CheckActorAreSameFaction(Actor* actorOne, Actor* actorTwo);
	Actor*				GetClosestVisibleActor(Actor* searchingActor);
						
	RaycastResult		RaycastAll(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance, Actor* firingActor = nullptr);
						
	void				DebugPossessNext();
						
	// Drawn by Game::RenderGame on top of everything once the courtyard falls.
	float				GetCourtyardWinFadeFraction() const;

	static bool			Event_OnKillAllActors(EventArgs& args);
	static bool			Event_OnDisplaySunSettings(EventArgs& args);
	static bool			Event_DebugControlLighting(EventArgs& args);
						
private:				
	
	void				DisplayTime();

	void				InitializeMapByImage(Image& mapImage);
	void				GenerateMapVerts();
	void				AddVertsForWall(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	void				AddVertsForFloor(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	void				AddVertsForCeiling(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	
	void				SunColors();
	void				UpdateLights();
	void				UpdateDirectionLights();
	void				UpdateSunIntensity();
	void				UpdateAmbientLighting();
	void				CheckAndFillLights();
	bool				IsLightInScreenSpace(Light const& light);

	void				ActorUpdate();
	void				PhysicsUpdate();
	void				ActorAudioUpdate();
	void				ManageDeadActors();
	void				SpawnNewPlayerIfPlayerControllerActorIsDead();
						
	void				CheckForCollisions();
	void				CheckActorVsActorCollision(Actor* collidingActor);
	void				CheckActorVsMapCollision(Actor* actor);
						
	void				RenderAllActors(Camera const& camera) const;
	void				RespawnDemons();
	void				SpawnAllActors();
	void				SpawnPlayer();
	void				AddActorToList(Actor* actor, std::vector<Actor*>& actorList);

	int					GetActorIndexInList(Actor* actor, ActorList const& actorList);
	bool				IsValidPosition(Vec3 const& position);
	std::vector<Tile>	GetEightSurroundingTiles(int posX, int posY);

	void				CheckGoalConditions();
	void				AwardCaptureRewards();
	void				ApplyPlayerProgressionToActor(PlayerController* player, Actor* actor);
	void				ApplyEnemyHealthBonus(Actor* actor);
	void				RestoreEnemyEmpoweredDamage(Actor* actor);

	// Not const: IntRange::IsOnRange is non-const in the Engine.
	bool				IsCaptureWindowOpen();
	float				GetHoursUntilCaptureWindowCloses() const;
	float				GetHoursUntilCaptureWindowOpens() const;
	int					GetCaptureWindowOpenHour() const;
	int					GetCaptureWindowCloseHour() const;
	bool				IsPlayerInCourtyard();
	Rgba8				GetFlashingTextColor(Rgba8 const& baseColor) const;
	void				DisplayCaptureWindowStatus();
	void				DisplayCourtyardStatus();
	void				DisplayTimedStatusMessages();
	void				AddTimedStatusMessage(std::string const& text, Rgba8 const& color, bool isFlashing);
	void				CheckDayChangeNotification();
	void				UpdateCourtyardWinSequence();

	RaycastResult		RaycastVsActors(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance, Actor* firingActor = nullptr);
	RaycastResult		RaycastVsCeiling(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance);
	RaycastResult		RaycastVsFloor(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance);
	RaycastResult		RaycastVsWalls(Vec3 const& startPosition, Vec3 const& fwdNormal, float distance);

public:

	Game*				m_game		= nullptr;
	MapDefinition*		m_mapDef	= nullptr;

	bool				m_didLevelJustStart = false;

	ActorList			m_allActors;
	ActorList			m_allSpawnPoints;
	Vec3				m_sunDirection		= Vec3(0.f, 0.f, 0.f);

// Event bool
	bool				m_displaySunSettings = false;
	bool				m_controlSun		 = false;
	std::vector<Vertex_PCU> m_textVerts;

private:
	unsigned int		m_currentUID = 0;

// Sun Settings
	float				m_sunIntensity		= 0.7f;
	float				m_ambientIntensity	= 1.f;
	float				m_sunDirectionYaw   = 0.f;
	float				m_sunDirectionPitch = 90.f;

	Light				m_directionalLight;
	Light				m_pointLight;
	std::vector<Light>	m_allLights;
	std::vector<Light>	m_mapLights;

	IntVec2				m_bounds;
	std::vector<Tile>	m_tiles;

// Timers
	Timer				m_physicsTimer;
	Timer				m_sunTimer;
	Timer				m_sunYawTimer;
	
//Renderer
	VertexBuffer*		m_vbo = nullptr;
	IndexBuffer*		m_ibo = nullptr;

	std::vector<unsigned int>		m_indexes;
	std::vector<Vertex_PCUTBN>		m_verts;
	std::vector<Rgba8>				m_colors;

 	FloatRange m_greenXGoalRange = FloatRange(1.f, 13.f);
	FloatRange m_greenYGoalRange = FloatRange(1.f, 13.f);
	
	FloatRange m_yellowXGoalRange = FloatRange(54.f, 63.f);
	FloatRange m_yellowYGoalRange = FloatRange(2.f, 7.f);

	FloatRange m_blueXGoalRange = FloatRange(2.f, 13.f);
	FloatRange m_blueYGoalRange = FloatRange(53.f, 63.f);

	FloatRange m_redXGoalRange = FloatRange(52.f, 63.f);
	FloatRange m_redYGoalRange = FloatRange(53.f, 63.f);

	FloatRange m_courtyardXGoalRange = FloatRange(21.f, 44.f);
	FloatRange m_courtyardYGoalRange = FloatRange(27.f, 44.f);

	IntRange m_noCaptureHourRange = IntRange(7, 21);

	bool	   m_isGreenCaptured		= false;
	bool	   m_isRedCaptured			= false;
	bool	   m_isBlueCaptured			= false;
	bool	   m_isYellowCaptured		= false;
	bool	   m_isCourtyardCaptured	= false;

	bool	   m_canCaptureObjective	= false;

	int		   m_numPlayersOnGreen	= 0;
	int		   m_numPlayersOnBlue	= 0;
	int		   m_numPlayersOnRed	= 0;
	int		   m_numPlayersOnYellow = 0;
	int		   m_numPlayersOnCourtyard = 0;

	float      m_greenCapturePercent	 = 0.f;
	float      m_blueCapturePercent		 = 0.f;
	float      m_redCapturePercent		 = 0.f;
	float      m_yellowCapturePercent	 = 0.f;
	float	   m_courtyardCapturePercent = 0.f;

	int		   m_numZonesCaptured = 0;

	int		   m_numDaysPassed = 0;
	int		   m_hours = 0;
	float	   m_timeOfDayHours = 0.f;
	float	   m_enemyHealthBonus = 0.f;

	Timer	   m_courtyardWinTimer;
	bool	   m_hasWipedEnemiesForCourtyardWin = false;
	bool	   m_areEnemiesEmpowered = false;
	int		   m_lastNotifiedDay = 0;

	// Transition latches so each objective banner fires once instead of every frame.
	bool	   m_wasCaptureWindowOpen = false;
	bool	   m_hasWarnedWindowClosing = false;
	bool	   m_hasWarnedWindowOpening = false;
	bool	   m_hasAnnouncedCourtyardReady = false;
	bool	   m_hasAnnouncedCourtyardCaptured = false;

	std::vector<TimedStatusMessage> m_statusMessages;


};