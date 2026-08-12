#include "Game/PlayerController.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/Game.hpp" 
#include "Game/Weapon.hpp" 
#include "Game/Map.hpp" 
#include "Game/GameCommon.hpp"
#include "Game/Controller.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

extern Game* g_game;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerController::PlayerController(int controllerID)
	: Controller(nullptr)
	, m_controllerID(controllerID)
{



}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerController::~PlayerController()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::Update()
{
	if(g_game->GetGameState() == GameState::PLAYING)
	{
		UpdateKeyboardInput();
		UpdateXboxInput();
	}

	HandleFreeFlyInput();

	if(m_map->m_game->GetNumPlayerControllers() == 1)
	{
		if(g_inputSystem->WasKeyJustPressed('F'))
		{
			ToggleControlMode();
		}

		if(g_inputSystem->WasKeyJustPressed('N'))
		{
			m_map->DebugPossessNext();
		}
	}

	if(m_deaths >= 3)
	{
		g_game->ChangeGameState(GameState::LOST);
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::UpdateCameras()
{
	UpdateWorldCamera();
	UpdateScreenCamera();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::UpdateWorldCamera()
{
	float fov = 60.f;

	Actor* controlledActor = m_map->GetActorByHandle(m_actorHandle);

	if(controlledActor)
	{
		fov = controlledActor->m_definition->m_cameraFovDegrees;
	}

	if(controlledActor && controlledActor->m_state == ActorState::DYING)
	{
		float lerpTimer = GetClampedZeroToOne(controlledActor->m_corpseTimer.GetElapsedFraction() * 2.f);

		m_position.z = Lerp(controlledActor->m_definition->m_eyeHeight, 0.1f, lerpTimer);
	}

	m_worldCamera.m_mode = Camera::eMode_Perspective;

	m_worldCamera.SetPositionAndOrientation(m_position, m_orientationDegrees);

	Mat44 cameraToRenderMatrix;

	cameraToRenderMatrix.SetIJKT3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 0.f));

	float aspect = m_worldCamera.m_viewportBounds.m_maxs.x / (m_worldCamera.m_viewportBounds.m_maxs.y - m_worldCamera.m_viewportBounds.m_mins.y);

	m_worldCamera.SetCameraToRenderTransform(cameraToRenderMatrix);
	m_worldCamera.SetPerspectiveView(aspect, fov, 0.1f, 100.f);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::UpdateScreenCamera()
{
	m_screenCamera.m_mode = Camera::eMode_Orthographic;

	m_screenCamera.SetOrthographicView(m_screenCamera.m_viewportBounds);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::ToggleControlMode()
{

	if(m_controlMode == ControlMode::FREEFLY)
	{
		if(!GetActor())
		{
			DebugAddMessage("No actor on map to possess", 5.f, Rgba8::RED);
			return;
		}
		m_controlMode = ControlMode::ACTOR_CONTROL;
		Possess(GetActor());
	}
	else if(m_controlMode == ControlMode::ACTOR_CONTROL)
	{
		Unpossess();
		m_controlMode = ControlMode::FREEFLY;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::RenderHUDInfoAndDeathOverlay()
{

	if(m_controlMode == ControlMode::FREEFLY)
	{
		return;
	}

	std::string healthText = Stringf("9999");

	Actor* controlledActor = GetActor();

	if(controlledActor)
	{
		healthText = Stringf("%d", GetClamped(static_cast<int>(controlledActor->m_health), 0, 100));
	}

	std::string killsText = Stringf("%d", m_kills);
	std::string deathsText = Stringf("%d", m_deaths);;

	std::vector<Vertex_PCU> hudVerts;

	float viewportHeight = m_screenCamera.m_viewportBounds.m_maxs.y - m_screenCamera.m_viewportBounds.m_mins.y;

	AABB2 hudAABB;
	hudAABB.m_mins = m_screenCamera.m_viewportBounds.m_mins;
	hudAABB.m_maxs = Vec2(m_screenCamera.m_viewportBounds.m_maxs.x, m_screenCamera.m_viewportBounds.m_mins.y + (viewportHeight * 0.15f));

	AABB2 killsBoxUVs;
	killsBoxUVs.m_mins = Vec2::ZERO;
	killsBoxUVs.m_maxs = Vec2(0.13f, 1.f);
	AABB2 killCountBox = hudAABB.GetBoxFromUVs(killsBoxUVs);

	AABB2 healthBoxUVs;
	healthBoxUVs.m_mins = Vec2(0.242f, 0.f);
	healthBoxUVs.m_maxs = Vec2(0.372f, 1.f);
	AABB2 healthBox = hudAABB.GetBoxFromUVs(healthBoxUVs);

	AABB2 deathsBoxUVs;
	deathsBoxUVs.m_maxs = Vec2::ONE;
	deathsBoxUVs.m_mins = deathsBoxUVs.m_maxs - Vec2(0.13f, 1.f);
	AABB2 deathsBox = hudAABB.GetBoxFromUVs(deathsBoxUVs);

	Vec2 alignment = Vec2(0.5f, 0.6f);
	float textScale = 1.f / static_cast<float>(g_game->GetNumPlayerControllers());
	float cellHeight = 80.f * textScale;
	float aspect = 0.8f;

	g_gameFont->AddVertsForTextInBox2D(hudVerts, healthText, healthBox,    cellHeight, Rgba8::WHITE, aspect, alignment, SHRINK_TO_FIT);
	g_gameFont->AddVertsForTextInBox2D(hudVerts, killsText,  killCountBox, cellHeight, Rgba8::WHITE, aspect, alignment, SHRINK_TO_FIT);
	g_gameFont->AddVertsForTextInBox2D(hudVerts, deathsText, deathsBox,	   cellHeight, Rgba8::WHITE, aspect, alignment, SHRINK_TO_FIT);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->DrawVertexArray(hudVerts);

	if(controlledActor && controlledActor->m_state == ActorState::DYING)
	{
		std::vector<Vertex_PCU> overlayVerts;
		AABB2 overlayBox = m_screenCamera.m_viewportBounds;
		Rgba8 overlayColor = Rgba8::GREY;
		float timeFraction = controlledActor->m_corpseTimer.GetElapsedFraction() * 2.f;

		overlayColor.a = static_cast<uchar>(Lerp(0.f, 50.f, GetClampedZeroToOne(timeFraction)));

		AddVertsForAABB2D(overlayVerts, overlayBox, overlayColor);

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(overlayVerts);	
	}

	if(controlledActor && controlledActor->m_map->m_game->m_isUIDebugMode)
	{
		std::vector<Vertex_PCU> debugVerts;

		AddVertsForAABB2D(debugVerts, hudAABB, Rgba8(255, 0, 255, 100));
		AddVertsForAABB2D(debugVerts, killCountBox, Rgba8(255, 0, 0, 200));
		AddVertsForAABB2D(debugVerts, deathsBox, Rgba8(255, 0, 0, 200));
		AddVertsForAABB2D(debugVerts, healthBox, Rgba8(255, 0, 0, 200));

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(debugVerts);

	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 PlayerController::GetForwardNormal() const
{
	Mat44 orientationMatrix = m_orientationDegrees.GetAsMatrix_IFwd_JLeft_KUp();
	return orientationMatrix.GetIBasis3D();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::UpdateKeyboardInput()
{
	if(m_controlMode != ControlMode::ACTOR_CONTROL || m_controllerID != -1)
	{
		return; 
	}

	Actor* controlledActor = m_map->GetActorByHandle(m_actorHandle);

	if(controlledActor && m_controlMode == ControlMode::ACTOR_CONTROL && static_cast<int>(controlledActor->m_state) < static_cast<int>(ActorState::DYING))
	{
		m_position = controlledActor->m_position + Vec3::UP * controlledActor->m_definition->m_eyeHeight;
		m_orientationDegrees = controlledActor->m_orientation;
	}

	float moveSpeed;

	if(controlledActor)
	{
		moveSpeed = controlledActor->m_definition->m_walkSpeed;
	}
	else
	{
		moveSpeed = g_gameConfigBlackboard.GetValue("playerMoveSpeed", 1.0f);
	}

	float turnSensitivity = 0.125f;

	m_orientationDegrees.m_yawDegrees += -(g_inputSystem->GetCursorClientDelta().x * turnSensitivity);
	m_orientationDegrees.m_pitchDegrees += g_inputSystem->GetCursorClientDelta().y * turnSensitivity;
	m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.f, 85.f);

	Vec3 fwdVector;
	Vec3 leftVector;
	Vec3 upVector;

	m_orientationDegrees.GetAsVectors_IFwd_JLeft_KUp(fwdVector, leftVector, upVector);

	bool wasKeyPressed = false;
	bool didJump = false;

	if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
	{
		if(controlledActor)
		{
			moveSpeed = controlledActor->m_definition->m_runSpeed;
		}
		else
		{
			moveSpeed = g_gameConfigBlackboard.GetValue("playerSprintSpeed", 10.f);
		}
	}

	if(controlledActor && static_cast<int>(controlledActor->m_state) < static_cast<int>(ActorState::DYING) && !m_map->m_game->m_gameClock->IsPaused())
	{
		controlledActor->m_orientation = m_orientationDegrees;

		Vec3 fwdXY = Vec3(fwdVector.x, fwdVector.y, 0.f).GetNormalized();

		Vec3 moveDirection = Vec3::ZERO;

		if(g_inputSystem->IsKeyDown('W') && controlledActor->m_isGrounded)
		{
			moveDirection += fwdXY;
			wasKeyPressed = true;
		}

		if(g_inputSystem->IsKeyDown('S') && controlledActor->m_isGrounded)
		{
			moveDirection -= fwdXY;
			wasKeyPressed = true;
		}

		if(g_inputSystem->IsKeyDown('A') && controlledActor->m_isGrounded)
		{
			moveDirection += leftVector;
			wasKeyPressed = true;
		}

		if(g_inputSystem->IsKeyDown('D') && controlledActor->m_isGrounded)
		{
			moveDirection -= leftVector;
			wasKeyPressed = true;
		}

		if(g_inputSystem->WasKeyJustPressed(' ') && !m_map->m_didLevelJustStart && controlledActor->m_isGrounded)
		{
			controlledActor->AddImpulse(Vec3::UP * controlledActor->m_definition->m_jumpHeight);
		}
		if(wasKeyPressed)
		{
			controlledActor->MoveInDirection(moveDirection.GetNormalized() * moveSpeed, didJump);
		}

		if(g_inputSystem->IsKeyDown(KEYCODE_LEFT_MOUSE))
		{
			controlledActor->Attack();
		}

		if(g_inputSystem->WasKeyJustPressed('1'))
		{
			controlledActor->EquipWeapon(0);
		}
		if(g_inputSystem->WasKeyJustPressed('2'))
		{
			controlledActor->EquipWeapon(1);
		}

		if(g_inputSystem->WasKeyJustPressed('3'))
		{
			controlledActor->EquipWeapon(2);
		}

		if(g_inputSystem->WasKeyJustPressed('4'))
		{
			controlledActor->EquipWeapon(3);
		}

		if(g_inputSystem->WasKeyJustPressed('5'))
		{
			controlledActor->EquipWeapon(4);
		}

		if(g_inputSystem->WasKeyJustPressed('6'))
		{
			controlledActor->EquipWeapon(5);
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_LEFT_ARROW) || g_inputSystem->GetWheelDelta() > 0.f)
		{
			controlledActor->CyclePrevWeapon();
		}

		if(g_inputSystem->WasKeyJustPressed(KEYCODE_RIGHT_ARROW) || g_inputSystem->GetWheelDelta() < 0.f)
		{
			controlledActor->CycleNextWeapon();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::UpdateXboxInput()
{
	if(m_controlMode != ControlMode::ACTOR_CONTROL || m_controllerID == -1)
	{
		return;
	}

	XboxController const& xboxController = g_inputSystem->GetController(m_controllerID);

	if(!xboxController.IsConntected())
	{
		return;
	}

	AnalogJoystick const& rightStick = xboxController.GetRightStick();
	AnalogJoystick const& leftStick = xboxController.GetLeftStick();
	float				  rightTrigger = xboxController.GetRightTrigger();

	Actor* controlledActor = m_map->GetActorByHandle(m_actorHandle);

	if(controlledActor && m_controlMode == ControlMode::ACTOR_CONTROL && static_cast<int>(controlledActor->m_state) < static_cast<int>(ActorState::DYING))
	{
		m_position = controlledActor->m_position;
		m_position.z = controlledActor->m_definition->m_eyeHeight;
		m_orientationDegrees = controlledActor->m_orientation;
	}

	float moveSpeed;

	if(controlledActor)
	{
		moveSpeed = controlledActor->m_definition->m_walkSpeed;
	}
	else
	{
		moveSpeed = g_gameConfigBlackboard.GetValue("playerMoveSpeed", 1.0f);
	}


	float turnSensitivity = 0.125f;

	m_orientationDegrees.m_yawDegrees -= (rightStick.GetPosition().x * turnSensitivity);
	m_orientationDegrees.m_pitchDegrees -= rightStick.GetPosition().y * turnSensitivity;
	m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.f, 85.f);

	Vec3 fwdVector;
	Vec3 leftVector;
	Vec3 upVector;

	m_orientationDegrees.GetAsVectors_IFwd_JLeft_KUp(fwdVector, leftVector, upVector);

	if(xboxController.IsButtonDown(XBOX_BUTTON_A) || xboxController.IsButtonDown(XBOX_BUTTON_LTHUMB))
	{
		if(controlledActor)
		{
			moveSpeed = controlledActor->m_definition->m_runSpeed;
		}
		else
		{
			moveSpeed = g_gameConfigBlackboard.GetValue("playerSprintSpeed", 10.f);
		}
	}

	if(controlledActor && static_cast<int>(controlledActor->m_state) < static_cast<int>(ActorState::DYING) && !m_map->m_game->m_gameClock->IsPaused())
	{
		controlledActor->m_orientation = m_orientationDegrees;

		Vec3 fwdXY = Vec3(fwdVector.x, fwdVector.y, 0.f).GetNormalized();

		if(leftStick.GetPosition().GetLengthSquared() > 0.1f * 0.1f)
		{
			Vec3 moveDirection = fwdXY * leftStick.GetPosition().y - leftVector * leftStick.GetPosition().x;
			moveDirection.Normalize();
			controlledActor->MoveInDirection(moveDirection * moveSpeed);
		}

		if(rightTrigger > 0.2f)
		{
			controlledActor->Attack();
		}

		if(g_inputSystem->WasKeyJustPressed(xboxController.WasButtonJustPressed(XBOX_BUTTON_X)))
		{
			controlledActor->EquipWeapon(0);
		}
		if(g_inputSystem->WasKeyJustPressed(xboxController.WasButtonJustPressed(XBOX_BUTTON_Y)))
		{
			controlledActor->EquipWeapon(1);
		}

		if(xboxController.WasButtonJustPressed(XBOX_BUTTON_DPAD_LEFT))
		{
			controlledActor->CyclePrevWeapon();
		}

		if(xboxController.WasButtonJustPressed(XBOX_BUTTON_DPAD_RIGHT))
		{
			controlledActor->CycleNextWeapon();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PlayerController::HandleFreeFlyInput()
{
	if(m_controlMode != ControlMode::FREEFLY)
	{
		return;
	}

	XboxController const& xboxController = g_inputSystem->GetController(0);
	AnalogJoystick const& rightStick = xboxController.GetRightStick();
	AnalogJoystick const& leftStick = xboxController.GetLeftStick();

	float moveSpeed;

	Actor* controlledActor = m_map->GetActorByHandle(m_actorHandle);

	if(controlledActor)
	{
		moveSpeed = controlledActor->m_definition->m_walkSpeed;
	}
	else
	{
		moveSpeed = g_gameConfigBlackboard.GetValue("playerMoveSpeed", 1.0f);
	}

	float turnSensitivity = 0.125f;

	m_orientationDegrees.m_yawDegrees += -(g_inputSystem->GetCursorClientDelta().x * turnSensitivity);
	m_orientationDegrees.m_pitchDegrees += g_inputSystem->GetCursorClientDelta().y * turnSensitivity;
	m_orientationDegrees.m_yawDegrees -= (rightStick.GetPosition().x * turnSensitivity);
	m_orientationDegrees.m_pitchDegrees -= rightStick.GetPosition().y * turnSensitivity;

	m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.f, 85.f);

	Vec3 fwdVector;
	Vec3 leftVector;
	Vec3 upVector;

	m_orientationDegrees.GetAsVectors_IFwd_JLeft_KUp(fwdVector, leftVector, upVector);

	if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
	{
		if(controlledActor)
		{
			moveSpeed = controlledActor->m_definition->m_runSpeed;
		}
		else
		{
			moveSpeed = g_gameConfigBlackboard.GetValue("playerSprintSpeed", 10.f);
		}
	}

	float deltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

	Vec3 moveDirection = Vec3::ZERO;

	if(g_inputSystem->IsKeyDown('W'))
	{
		moveDirection += fwdVector;
	}

	if(g_inputSystem->IsKeyDown('S'))
	{
		moveDirection -= fwdVector;
	}

	if(g_inputSystem->IsKeyDown('A'))
	{
		moveDirection += leftVector;
	}

	if(g_inputSystem->IsKeyDown('D'))
	{
		moveDirection -= leftVector;
	}

	if(leftStick.GetPosition().GetLengthSquared() > 0.1f * 0.1f)
	{
		moveDirection = fwdVector * leftStick.GetPosition().y - leftVector * leftStick.GetPosition().x;
	}

	moveDirection.Normalize();

	m_position += moveDirection * moveSpeed * deltaSeconds;

	if(g_inputSystem->IsKeyDown('E') || xboxController.IsButtonDown(XBOX_BUTTON_RIGHT_SHOULDER))
	{
		m_position.z += moveSpeed * deltaSeconds;
	}

	if(g_inputSystem->IsKeyDown('Q') || xboxController.IsButtonDown(XBOX_BUTTON_LEFT_SHOULDER))
	{
		m_position.z -= moveSpeed * deltaSeconds;
	}

}
