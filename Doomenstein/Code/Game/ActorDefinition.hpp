#pragma once
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Light.hpp"
#include "Engine/Renderer/SpriteAnimDefintion.hpp"
#include "ThirdParty/tinyXML2/tinyxml2.h"
#include <vector>
#include <string>
#include <map>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef tinyxml2::XMLElement	XmlElement;

class Texture;
class Shader;
class SpriteSheet;
struct IntRange;

typedef size_t SoundID;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class Faction
{
	NONE,

	MARINE,
	NEUTRAL,
	DEMON,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct AnimationGroup
{
public:
	AnimationGroup() = default;
	explicit AnimationGroup(XmlElement const& actorDefElement, SpriteSheet const& spriteSheet);

	SpriteAnimDefinition* GetAnimationDefinitionBasedOnViewingDirection(Vec3 const& viewingDriection);

public:

	std::string	m_name;
	
	bool m_scaleBySpeed = false;

	std::map<std::string, SpriteAnimDefinition*> m_animDefinitionBasedOnDirection;
};

struct SoundGroup
{
	std::string m_name;
	SoundID m_id;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class ActorDefinition
{
public:

	ActorDefinition() = default;
	explicit ActorDefinition(XmlElement const& actorDefElement);

	~ActorDefinition() = default;

	static void InitializeActorDefinition();
	static void InitializeProjectileActorDefinition();

	void InitializeCollisionValues(XmlElement const& actorDefElement);
	void InitializePhysicsValues(XmlElement const& actorDefElement);
	void InitializeCameraValues(XmlElement const& actorDefElement);
	void InitializeAIValues(XmlElement const& actorDefElement);
	void InitializeWeaponInventory(XmlElement const& actorDefElement);
	void InitializeVisuals(XmlElement const& visualDefElement);
	void InitializeAudio(XmlElement const& audioDefElement);
	void SetFaction(std::string factionName);
	void SetBillboardType(std::string billboardType);

	AnimationGroup GetAnimationByName(std::string animName);
	SoundID GetSoundByName(std::string soundName);

public:
	
	static std::vector<ActorDefinition> s_actorDefinitions;

	// base
	std::string m_name;
	Faction     m_faction		 = Faction::NEUTRAL;
	float		m_health		 = 1.f;
	float		m_corpseLifetime = 2.f;
	float		m_lifetime		 = -1.f;
	bool		m_visible		 = false;
	bool		m_canBePossessed = true;
	bool		m_dieOnSpawn	 = false;

	// collision
	float		m_physicsRadius		 = 0.f;
	float		m_physicsHeight		 = 0.f;
	bool		m_collidesWithActors = false;
	bool		m_collidesWithWalls  = false;
	bool		m_dieOnCollide		 = false;
	float		m_impulseOnCollide	 = 0.f;
	FloatRange  m_damageOnCollide	 =  FloatRange::ZERO;
	
	// physics
	bool		m_physicsSimulated	 = false;
	bool		m_effectedByGravity	 = true;
	float		m_walkSpeed			 = 0.f;
	float		m_runSpeed			 = 0.f;
	float		m_drag				 = 0.f;
	float		m_turnSpeed			 = 0.f;
	float		m_jumpHeight		 = 0.f;
	float		m_impulseDampening = 1.f;

	// camera
	float		m_eyeHeight			 = 0.f;
	float		m_cameraFovDegrees	 = 60.f;

	// AI
	bool		m_aiEnabled			 = false;
	float		m_sightRadius		 = 0.f;
	float		m_sightAngle		 = 0.f;

	// weapon
	std::vector<std::string> m_weaponInventory;

	// visuals
	bool		   m_renderLit		= false;
	bool		   m_renderRounded	= false;
	Vec2		   m_pivot			= Vec2(0.5f, 0.5f);
	Vec2		   m_spriteSize	    = Vec2::ONE;
	IntVec2		   m_cellCount		= IntVec2::ONE;
	Shader*		   m_shader			= nullptr;
	Texture*	   m_texture	    = nullptr;
	SpriteSheet*   m_spriteSheet;
	BillboardType  m_billboardType	= BillboardType::NONE;
	Rgba8		   m_tint			= Rgba8::WHITE;
	std::vector<AnimationGroup> m_animGroups;
	std::vector<SoundGroup>		m_soundGroups;

	bool m_isLightSource = false;
	Light m_light;

};