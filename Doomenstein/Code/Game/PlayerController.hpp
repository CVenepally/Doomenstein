#pragma once

#include "Engine/Renderer/Camera.hpp"
#include "Game/Controller.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ActorHandle;
class NamedStrings;

typedef NamedStrings EventArgs;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class ControlMode
{
	NONE,

	ACTOR_CONTROL,
	FREEFLY,

	COUNT
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class PlayerController: public Controller
{
public:

	PlayerController(int controllerID);
	~PlayerController();

	virtual void Update() override;
	void UpdateCameras();
	void UpdateWorldCamera();
	void UpdateScreenCamera();
	void ToggleControlMode();

	void RenderHUDInfoAndDeathOverlay();

	Vec3 GetForwardNormal() const;

private:

	void UpdateKeyboardInput();
	void UpdateXboxInput();
	void HandleFreeFlyInput();

public:

	Vec3		m_position;
	EulerAngles m_orientationDegrees;
	Camera		m_worldCamera;
	Camera		m_screenCamera;
	int			m_controllerID;

	int			m_kills = 0;
	int			m_deaths = 0;

	ControlMode m_controlMode = ControlMode::ACTOR_CONTROL;

};