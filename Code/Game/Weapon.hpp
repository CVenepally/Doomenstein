#pragma once

#include "Engine/Core/Timer.hpp"
#include "Game/Actor.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class WeaponDefinition;
class PlayerController;
class SpriteAnimDefinition;

struct Vec3;
typedef size_t SoundID;
typedef size_t SoundPlaybackID;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum WeaponState
{
	IDLE,
	ATTACK
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Weapon
{
public:

	Weapon(WeaponDefinition* weaponDefinition, Actor* owner = nullptr);
	~Weapon();

	void Fire();

	// Combat upgrades live on the firing player, so AI weapons stay unscaled.
	PlayerController* GetOwningPlayerController() const;

	void FirePistol();
	void FirePlasma();
	void Melee();

	void Render(Camera const& camera);
	void RenderHUD(Camera const& camera) const;
	void RenderReticle(Camera const& camera) const;
	void RenderWeapon(Camera const& camera);

	SpriteAnimDefinition* GetSpriteAnimDefByState();
	SoundID				  GetSoundIDForCurrentState();

	Vec3 GetRandomDirectionInCone(float offset);

	void SetAnimationTimerByState();
	void SetWeaponState(WeaponState newState);

public:

	Actor*			  m_owner = nullptr;
	WeaponDefinition* m_weaponDefinition = nullptr;

	// Seeded from the definition so a single wielder can be buffed without touching the
	// shared, static WeaponDefinition that every other actor reads.
	FloatRange		  m_meleeDamage;
	WeaponState		  m_state = IDLE;
	Timer			  m_refireTimer;
	Timer			  m_animationTimer;

	SoundPlaybackID	  m_currentAudioID = static_cast<SoundPlaybackID>(-1);

};