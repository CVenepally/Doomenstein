#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Controller.hpp"
#include "Game/GameCommon.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ActorHandle;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class ControlMode
{
	NONE,

	ACTOR_CONTROL,
	FREEFLY,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// A short-lived banner shown above the HUD bar when the player earns an upgrade.
struct UpgradeNotification
{
	std::string m_text;
	Timer		m_timer;
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

	void AddUpgradeNotification(std::string const& text);

	float GetMaxHealth() const;

	// Same value as GetMaxHealth, but for a known actor. Use this when the controller's
	// m_map may not be wired up yet, such as during Map construction.
	float GetMaxHealthFor(Actor* actor) const;

	Vec3 GetForwardNormal() const;

private:

	void UpdateKeyboardInput();
	void UpdateXboxInput();
	void HandleFreeFlyInput();
	void RenderUpgradeNotifications();
	void PruneExpiredUpgradeNotifications();

public:

	Vec3		m_position;
	EulerAngles m_orientationDegrees;
	Camera		m_worldCamera;
	Camera		m_screenCamera;
	int			m_controllerID;

	int			m_kills = 0;
	int			m_deaths = 0;

	// Survives respawns, so capture upgrades persist across lives. Added on top of the
	// possessed actor's definition health rather than replacing it, so the ceiling still
	// tracks ActorDefinitions.xml.
	float		m_healthUpgradeBonus = 0.f;
	bool		m_isPlasmaRifleUnlocked = false;
	float		m_moveSpeedMultiplier = 1.f;

	// Combat upgrades, stacking once per KILLS_PER_COMBAT_UPGRADE kills.
	float		m_weaponDamageMultiplier	 = 1.f;
	float		m_projectileSpeedMultiplier	 = 1.f;
	float		m_projectileSpreadMultiplier = 1.f;

	std::vector<UpgradeNotification> m_upgradeNotifications;

	ControlMode m_controlMode = ControlMode::ACTOR_CONTROL;

};