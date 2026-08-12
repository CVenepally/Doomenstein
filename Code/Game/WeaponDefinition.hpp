#pragma once

#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Vec2.hpp"
#include "ThirdParty/tinyXML2/tinyxml2.h"
#include <vector>
#include <string>
#include <map>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef tinyxml2::XMLElement	XmlElement;

class Texture;
class Shader;
class SpriteSheet;
class SpriteAnimDefinition;

struct SoundGroup;
typedef size_t SoundID;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class WeaponDefinition
{
public:

	WeaponDefinition() = default;
	explicit WeaponDefinition(XmlElement const& weaponDefElement);

	static void InitializeWeaponDefinition();
	void InitializeHUD(XmlElement const& hudElement);
	void InitializeSounds(XmlElement const& soundElement);

	SpriteAnimDefinition* GetAnimationSpriteDefByName(std::string name);
	SoundID GetSoundIDByName(std::string soundName);

	~WeaponDefinition() = default;

public:

	static std::vector<WeaponDefinition> s_weaponDefinitions;

	std::string m_name;
	
	float	   m_refireTime = 0.f;
	float	   m_rayCount	= 0.f;
	float	   m_rayCone	= 0.f;
	float	   m_rayImpulse = 0.f;
	float	   m_rayRange	= 0.f;
	FloatRange m_rayDamage	= FloatRange::ZERO;

	int			m_projectileCount = 0;
	float		m_projectileCone  = 0.f;
	float		m_projectileSpeed = 0.f;
	std::string m_projectileActorName;

	int		   m_meleeCount	  = 0;
	float	   m_meleeRange	  = 0.f;
	float	   m_meleeArc	  = 0.f;
	float	   m_meleeImpulse = 0.f;
	FloatRange m_meleeDamage  = FloatRange::ZERO;

	// HUD
	Shader*					m_shader		 = nullptr;
	Texture*				m_hudTexture     = nullptr;
	Texture*				m_reticleTexture = nullptr;
	Texture*				m_spriteSheetTexture	 = nullptr;

	std::map<std::string, SpriteAnimDefinition*>	m_weaponAnimationsBasedOnNames;

	Vec2					m_reticleSize;
	Vec2					m_spriteSize;
	Vec2					m_pivot;

	std::vector<SoundGroup> m_soundGroups;
};