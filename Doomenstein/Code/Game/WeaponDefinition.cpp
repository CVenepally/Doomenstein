#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteAnimDefintion.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/ActorDefinition.hpp"

extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudioSystem;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<WeaponDefinition> WeaponDefinition::s_weaponDefinitions;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
WeaponDefinition::WeaponDefinition(XmlElement const& weaponDefElement)
{
	m_name		 = ParseXmlAttribute(weaponDefElement, "name",		 m_name);
	m_refireTime = ParseXmlAttribute(weaponDefElement, "refireTime", m_refireTime);
	m_rayCount   = ParseXmlAttribute(weaponDefElement, "rayCount",	 m_rayCount);
	m_rayCone    = ParseXmlAttribute(weaponDefElement, "rayCone",	 m_rayCone);
	m_rayRange   = ParseXmlAttribute(weaponDefElement, "rayRange",   m_rayRange);
	m_rayImpulse = ParseXmlAttribute(weaponDefElement, "rayImpulse", m_rayImpulse);
	m_rayDamage  = ParseXmlAttribute(weaponDefElement, "rayDamage",  m_rayDamage);
	
	m_projectileCount     = ParseXmlAttribute(weaponDefElement, "projectileCount", m_projectileCount);
	m_projectileCone      =	ParseXmlAttribute(weaponDefElement, "projectileCone",  m_projectileCone);
	m_projectileSpeed	  =	ParseXmlAttribute(weaponDefElement, "projectileSpeed", m_projectileSpeed);
	m_projectileActorName =	ParseXmlAttribute(weaponDefElement, "projectileActor", m_projectileActorName);

	m_meleeCount   = ParseXmlAttribute(weaponDefElement, "meleeCount",   m_meleeCount);
	m_meleeRange   = ParseXmlAttribute(weaponDefElement, "meleeRange",   m_meleeRange);
	m_meleeArc	   = ParseXmlAttribute(weaponDefElement, "meleeArc",     m_meleeArc);
	m_meleeImpulse = ParseXmlAttribute(weaponDefElement, "meleeImpulse", m_meleeImpulse);
	m_meleeDamage  = ParseXmlAttribute(weaponDefElement, "meleeDamage",  m_meleeDamage);

	XmlElement const* childElement = weaponDefElement.FirstChildElement();
	
	while(childElement)
	{
		std::string elementName = childElement->Name();

		if(elementName == "HUD")
		{
			InitializeHUD(*childElement);
		}
		else if(elementName == "Sounds")
		{
			InitializeSounds(*childElement);
		}

		childElement = childElement->NextSiblingElement();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void WeaponDefinition::InitializeWeaponDefinition()
{

	XmlDocument weaponDef;
	char const* filePath = "Data/Definitions/WeaponDefinitions.xml"; // probably move all of these to GameConfig? Easy management?
	XmlResult result = weaponDef.LoadFile(filePath);

	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Could not open WeaponDefinitions.xml");

	XmlElement* rootElement = weaponDef.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Failed to access WeaponDefinitions root element");

	XmlElement* weaponDefElement = rootElement->FirstChildElement();

	while(weaponDefElement)
	{
		std::string elementName = weaponDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "WeaponDefinition", "Child Element should be WeaponDefinition");

		WeaponDefinition newWeaponDefintition = WeaponDefinition(*weaponDefElement);
		s_weaponDefinitions.push_back(newWeaponDefintition);

		weaponDefElement = weaponDefElement->NextSiblingElement();
	}


}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void WeaponDefinition::InitializeHUD(XmlElement const& hudElement)
{
	std::string hudTextureName;
	hudTextureName = ParseXmlAttribute(hudElement, "baseTexture", hudTextureName);
	m_hudTexture = g_theRenderer->CreateOrGetTextureFromFile(hudTextureName.c_str());

	std::string reticleTexture;
	reticleTexture = ParseXmlAttribute(hudElement, "reticleTexture", reticleTexture);
	m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(reticleTexture.c_str());

	m_reticleSize = ParseXmlAttribute(hudElement, "reticleSize", m_reticleSize);
	m_spriteSize  = ParseXmlAttribute(hudElement, "spriteSize", m_spriteSize);
	m_pivot		  = ParseXmlAttribute(hudElement, "spritePivot", m_pivot);

	XmlElement const* animElement = hudElement.FirstChildElement();

	while(animElement)
	{
		std::string elementName = animElement->Name();

		GUARANTEE_OR_DIE(elementName == "Animation", "Invalid Name: %s for Animation Definition under HUD. Rename it to Animation");

		IntVec2 cellCount = ParseXmlAttribute(*animElement, "cellCount", IntVec2::ZERO);

		std::string shaderName;
		shaderName = ParseXmlAttribute(*animElement, "shader", shaderName);
		m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str());

		std::string spriteSheetTextureName;
		spriteSheetTextureName = ParseXmlAttribute(*animElement, "spriteSheet", spriteSheetTextureName);
		m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetTextureName.c_str());

		SpriteSheet* spriteSheet = new SpriteSheet(*m_spriteSheetTexture, cellCount);
		
		float secondsPerFrame = 0.f;
		int	  startIndex = -1;
		int	  endIndex = -1;

		secondsPerFrame = ParseXmlAttribute(*animElement, "secondsPerFrame", secondsPerFrame);
		startIndex	    = ParseXmlAttribute(*animElement, "startFrame", startIndex);
		endIndex		= ParseXmlAttribute(*animElement, "endFrame", endIndex);

		float fps = 1.f / secondsPerFrame;

		SpriteAnimDefinition* animation = new SpriteAnimDefinition(*spriteSheet, startIndex, endIndex, fps, SpriteAnimPlaybackType::ONCE);

		std::string animName;
		animName = ParseXmlAttribute(*animElement, "name", animName);

		m_weaponAnimationsBasedOnNames[animName] = animation;

		animElement = animElement->NextSiblingElement();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void WeaponDefinition::InitializeSounds(XmlElement const& audioDefElement)
{
	XmlElement const* soundElement = audioDefElement.FirstChildElement();

	while(soundElement)
	{

		std::string soundName;
		std::string soundFilePath;

		soundName = ParseXmlAttribute(*soundElement, "sound", soundName);
		soundFilePath = ParseXmlAttribute(*soundElement, "name", soundName);

		SoundID soundID = g_theAudioSystem->CreateOrGetSound(soundFilePath, FMOD_3D);

		SoundGroup soundGroup;
		soundGroup.m_name = soundName;
		soundGroup.m_id = soundID;

		m_soundGroups.push_back(soundGroup);

		soundElement = soundElement->NextSiblingElement();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SpriteAnimDefinition* WeaponDefinition::GetAnimationSpriteDefByName(std::string name)
{
	auto def = m_weaponAnimationsBasedOnNames.find(name);

	if(def == m_weaponAnimationsBasedOnNames.end())
	{
		ERROR_RECOVERABLE(Stringf("Animation Definition For %s does not exist; WeaponDefinition::GetAnimationSpriteDefByName)", name.c_str()));
	}

	return def->second;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SoundID WeaponDefinition::GetSoundIDByName(std::string soundName)
{
	for(SoundGroup currentGroup : m_soundGroups)
	{
		if(currentGroup.m_name == soundName)
		{
			return currentGroup.m_id;
		}
	}

	return MISSING_SOUND_ID;

}
