#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DebugRender.hpp"

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/DevConsole.hpp"

class App;
class Actor;

extern App* g_theApp;
extern Renderer* g_theRenderer;
extern InputSystem* g_inputSystem;
extern AudioSystem* g_theAudioSystem;
extern Window* g_theWindow;
extern BitmapFont* g_gameFont;
extern RandomNumberGenerator g_numGenerator;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Deaths a player is allowed before the run is lost.
constexpr int	PLAYER_LIVES			 = 5;

// Health a freshly spawned Marine starts with, before any capture upgrades.
constexpr float PLAYER_BASE_MAX_HEALTH	 = 100.f;

// Zone-capture counts that grant progression rewards, handled in AwardCaptureRewards.
constexpr int	PLASMA_UNLOCK_MILESTONE			 = 2;
constexpr int	CAPTURE_UPGRADE_MILESTONE_FIRST	 = 1;
constexpr int	CAPTURE_UPGRADE_MILESTONE_SECOND = 4;

// The later health milestone is worth more, so deep captures stay worth pushing for.
constexpr float CAPTURE_HEALTH_UPGRADE_FIRST  = 25.f;
constexpr float CAPTURE_HEALTH_UPGRADE_SECOND = 50.f;

// Movement reward. Applied only to player input speed, never to AIController, which
// reads m_runSpeed straight off the shared definition.
constexpr int	MOVEMENT_UPGRADE_MILESTONE	= 3;
constexpr float MOVEMENT_UPGRADE_MULTIPLIER = 1.3f;

// Enemy scaling granted at the final capture milestone.
constexpr int	ENEMY_HEALTH_UPGRADE_MILESTONE = 4;
constexpr float ENEMY_HEALTH_UPGRADE		   = 50.f;

// Combat reward, granted every KILLS_PER_COMBAT_UPGRADE kills and stacking.
constexpr int	KILLS_PER_COMBAT_UPGRADE	   = 15;
constexpr float COMBAT_DAMAGE_MULTIPLIER	   = 1.10f;
constexpr float COMBAT_PROJECTILE_SPEED_MULTIPLIER = 1.10f;
constexpr float COMBAT_PROJECTILE_SPREAD_MULTIPLIER = 0.90f;

// Capture-window messaging. The window hours themselves are derived from
// Map::m_noCaptureHourRange so there is only one source of truth for them.
constexpr float CAPTURE_CLOSE_WARNING_HOURS = 5.f;
constexpr float CAPTURE_OPEN_WARNING_HOURS	= 4.f;
constexpr float CAPTURE_COUNTDOWN_HOURS		= 0.5f;

// Courtyard victory sequence: fade to white, wiping the map partway through.
constexpr double COURTYARD_WIN_FADE_SECONDS	 = 6.0;
constexpr float  COURTYARD_WIN_KILL_FRACTION = 0.3f;

// How long an objective banner stays under the clock. The capture-window countdown is
// deliberately exempt and renders continuously.
constexpr double OBJECTIVE_MESSAGE_DURATION = 5.0;

// Cycles per second for flashing on-screen status text.
constexpr float STATUS_TEXT_FLASH_HZ = 1.5f;

// How long an "upgrade obtained" banner sits above the HUD bar.
constexpr double UPGRADE_NOTIFICATION_DURATION = 5.0;

// Hour of day the map's point lights switch on. They stay lit until the capture
// window closes at m_noCaptureHourRange.m_min (07:00).
constexpr float POINT_LIGHTS_ON_HOUR	 = 17.5f;

struct RaycastResult
{
	RaycastResult3D m_rayResult;
	Actor* m_hitActor = nullptr;
};