#pragma once

#include "Engine/Core/Timer.hpp"
#include "Game/Actor.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class WeaponDefinition;
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
	WeaponState		  m_state = IDLE;
	Timer			  m_refireTimer;
	Timer			  m_animationTimer;

	SoundPlaybackID	  m_currentAudioID = static_cast<SoundPlaybackID>(-1);

};