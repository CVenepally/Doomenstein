#pragma once

#include "Engine/Math/Cylinder3D.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/Light.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/ActorHandle.hpp"

#include <vector>
#include <string>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Map;
class SpawnInfo;
class Camera;
class Weapon;
class ActorDefinition;
class WeaponDefinition;
class PlayerController;
class Controller;
class AIController;
class SpriteDefinition;
class SpriteAnimDefinition;

struct Vertex_PCU;
struct Vertex_PCUTBN;
struct AnimationGroup;

typedef size_t SoundPlaybackID;
typedef size_t SoundID;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

enum class ActorState
{
	WALKING,
	ATTACKING,
	HURTING,
	DYING,
	DEAD
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Actor
{

public:
	Actor(Map* map, std::string const& m_name, Vec3 const& m_position, EulerAngles const& m_orientation, ActorHandle const& actorHandle, Actor* owner = nullptr);
	~Actor();

	void Update();

	void DeathStateUpdate();

	void PhysicsUpdate();

	void Render(Camera const& camera);
	void RenderDepth();
	Mat44 GetModelMatrix() const;

	Vec3 GetForwardVector();
	Vec3 GetUpVector();
	Vec3 GetLeftVector();

	void FillActorVerts();

	void EquipWeapon(int weaponIndex);
	void CycleNextWeapon();
	void CyclePrevWeapon();
	void AddToInventory(Weapon* weapon);
	void Attack();

	void MoveInDirection(Vec3 const& direction, bool didJump = false);
	void AddForce(Vec3 const& force);
	void AddImpulse(Vec3 const& impulse);

	void TurnInDirection(float angle, float turnRate);

	void OnCollide(Actor* collidingActor = nullptr);
	void OnPossessed(Controller* controller);
	void OnUnpossessed(Controller* controller);

	void TakeDamage(float damage, Actor* attackingActor);
	void IncrementPlayerKillsOnAttackingPlayer(Actor* attackingActor);

	void SetAnimationIfViewChanged(Camera const& camera);
	void SetSpawnState();

	SpriteDefinition GetAnimationSpriteDef();
	AnimationGroup   GetAnimationGroupByState(ActorState state);

	void SetActorState(ActorState newState);
	bool IsAlive();

	Vec3 GetEyePosition();
	Vec3 GetViewingDirectionFromCamera(Camera const& camera);

	SoundID GetSoundIDForCurrentState();
	void	UpdateSoundPosition();
public:

	ActorDefinition* m_definition = nullptr;

	Actor* m_owner = nullptr;
	Map*   m_map   = nullptr;
	Rgba8  m_color = Rgba8::WHITE;

	Weapon*				 m_currentWeapon = nullptr;
	std::vector<Weapon*> m_weaponInventory;

	ActorHandle m_handle;

	float m_health;
	
	std::vector<Vertex_PCUTBN> m_verts;
	std::vector<Vertex_PCU>	   m_unlitVerts;
	std::vector<unsigned int>  m_indexes;

	Vec3		m_position;
	EulerAngles m_orientation;
	Vec3		m_acceleration;
	Vec3		m_velocity;
	bool		m_isGrounded = true;
	ActorState    m_state = ActorState::WALKING;
	ActorState    m_defaultState;

	Controller*	  m_possessedController = nullptr;
	AIController* m_aiController	    = nullptr;

	Timer		m_corpseTimer;
	Timer		m_lifetimeTimer;
	Timer		m_animationTimer;

	Light		m_light;

	SpriteAnimDefinition* m_actorAnimation = nullptr;
	bool m_scaleAnimationBySpeed = false;

	SoundPlaybackID m_currentAudioID = static_cast<SoundPlaybackID>(-1);

};