#include "Game/ActorDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"

extern AudioSystem* g_theAudioSystem;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<ActorDefinition> ActorDefinition::s_actorDefinitions;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ActorDefinition::ActorDefinition(XmlElement const& actorDefElement)
{
	m_name			 = ParseXmlAttribute(actorDefElement, "name", m_name);
	
	std::string faction = ParseXmlAttribute(actorDefElement, "faction", "Neutral");
	SetFaction(faction);

	m_health		 = ParseXmlAttribute(actorDefElement, "health", m_health);
	m_corpseLifetime = ParseXmlAttribute(actorDefElement, "corpseLifetime", m_corpseLifetime);
	m_lifetime	     = ParseXmlAttribute(actorDefElement, "lifetime", m_lifetime);
	m_visible		 = ParseXmlAttribute(actorDefElement, "visible", m_visible);
	m_canBePossessed = ParseXmlAttribute(actorDefElement, "canBePossessed", m_canBePossessed);
	
	XmlElement const* childDefElements = actorDefElement.FirstChildElement();

	while(childDefElements)
	{
		std::string name = childDefElements->Name();

		if(name == "Collision")
		{
			InitializeCollisionValues(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}
		else if(name == "Physics")
		{
			InitializePhysicsValues(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();

		}
		else if(name == "Camera")
		{
			InitializeCameraValues(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();

		}
		else if(name == "AI")
		{
			InitializeAIValues(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}
		else if(name == "Inventory")
		{
			InitializeWeaponInventory(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}
		else if(name == "Visuals")
		{
			InitializeVisuals(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}		
		else if(name == "Sounds")
		{
			InitializeAudio(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}
		else if(name == "Light")
		{
			m_isLightSource = ParseXmlAttribute(*childDefElements, "isLightSource", false);
 			m_light = Light::CreateFromXML(*childDefElements);
			childDefElements = childDefElements->NextSiblingElement();
		}
		else
		{
			childDefElements = childDefElements->NextSiblingElement();
			continue;
		}
	}
		
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeActorDefinition()
{
	XmlDocument actorDef;
	char const* filePath = "Data/Definitions/ActorDefinitions.xml"; // probably move all of these to GameConfig? Easy management?
	XmlResult result = actorDef.LoadFile(filePath);

	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Could not open ActorDefinitions.xml");

	XmlElement* rootElement = actorDef.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Failed to access ActorDefintions root element");

	XmlElement* actorDefElement = rootElement->FirstChildElement();

	while(actorDefElement)
	{
		std::string elementName = actorDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "ActorDefinition", "Child Element should be Actor Definition");

		ActorDefinition newActorDefintition = ActorDefinition(*actorDefElement);
		s_actorDefinitions.push_back(newActorDefintition);

		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeProjectileActorDefinition()
{
	XmlDocument projectileActorDef;
	char const* filePath = "Data/Definitions/ProjectileActorDefinitions.xml"; // probably move all of these to GameConfig? Easy management?
	XmlResult result = projectileActorDef.LoadFile(filePath);

	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Could not open ProjectileActorDefinitions.xml");

	XmlElement* rootElement = projectileActorDef.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Failed to access ProjectileActorDefintions root element");

	XmlElement* projectileActorDefElement = rootElement->FirstChildElement();

	while(projectileActorDefElement)
	{
		std::string elementName = projectileActorDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "ActorDefinition", "Child Element should be Actor Definition");

 		ActorDefinition newProjectileActorDefintition = ActorDefinition(*projectileActorDefElement);
		s_actorDefinitions.push_back(newProjectileActorDefintition);

		projectileActorDefElement = projectileActorDefElement->NextSiblingElement();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeCollisionValues(XmlElement const& actorDefElement)
{
	m_physicsRadius		 = ParseXmlAttribute(actorDefElement, "radius", m_physicsRadius);
	m_physicsHeight		 = ParseXmlAttribute(actorDefElement, "height", m_physicsHeight);
	m_collidesWithActors = ParseXmlAttribute(actorDefElement, "collidesWithActors", m_collidesWithActors);
	m_collidesWithWalls  = ParseXmlAttribute(actorDefElement, "collidesWithWorld", m_collidesWithWalls);
	m_dieOnCollide		 = ParseXmlAttribute(actorDefElement, "dieOnCollide", m_dieOnCollide);
	m_impulseOnCollide	 = ParseXmlAttribute(actorDefElement, "impulseOnCollide", m_impulseOnCollide);
	m_damageOnCollide	 = ParseXmlAttribute(actorDefElement, "damageOnCollide", m_damageOnCollide);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializePhysicsValues(XmlElement const& actorDefElement)
{
	m_physicsSimulated  = ParseXmlAttribute(actorDefElement, "simulated", m_physicsSimulated);
	m_effectedByGravity	= ParseXmlAttribute(actorDefElement, "effectedByGravity", m_effectedByGravity);
	m_walkSpeed			= ParseXmlAttribute(actorDefElement, "walkSpeed", m_walkSpeed);
	m_runSpeed			= ParseXmlAttribute(actorDefElement, "runSpeed", m_runSpeed);
	m_drag				= ParseXmlAttribute(actorDefElement, "drag", m_drag);
	m_turnSpeed			= ParseXmlAttribute(actorDefElement, "turnSpeed", m_turnSpeed);
	m_jumpHeight		= ParseXmlAttribute(actorDefElement, "jumpHeight", m_jumpHeight);
	m_impulseDampening = ParseXmlAttribute(actorDefElement, "impulseDampen", m_impulseDampening);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeCameraValues(XmlElement const& actorDefElement)
{
	m_eyeHeight		   = ParseXmlAttribute(actorDefElement, "eyeHeight", m_eyeHeight);
	m_cameraFovDegrees = ParseXmlAttribute(actorDefElement, "cameraFOV", m_cameraFovDegrees);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeAIValues(XmlElement const& actorDefElement)
{
	m_aiEnabled   = ParseXmlAttribute(actorDefElement, "aiEnabled", m_aiEnabled);
	m_sightRadius = ParseXmlAttribute(actorDefElement, "sightRadius", m_sightRadius);
	m_sightAngle  = ParseXmlAttribute(actorDefElement, "sightAngle", m_sightAngle);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeWeaponInventory(XmlElement const& actorDefElement)
{
	XmlElement const* weaponNameElement = actorDefElement.FirstChildElement();

	while(weaponNameElement)
	{
		std::string name = weaponNameElement->Name();
		GUARANTEE_OR_DIE(name == "Weapon", "Failed to access Weapon Element under Inventory. Element does not exist or named something other than \"Weapon\"\n \
						 (ActorDefinition.cpp | InitializeWeaponInventory()");
		
		std::string weaponName;
		weaponName = ParseXmlAttribute(*weaponNameElement, "name", weaponName);
		m_weaponInventory.push_back(weaponName);

		weaponNameElement = weaponNameElement->NextSiblingElement();

	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeVisuals(XmlElement const& visualDefElement)
{
	std::string billboardString;
	std::string shaderName;
	std::string spriteSheetName;

	m_spriteSize	= ParseXmlAttribute(visualDefElement, "size",		   m_spriteSize);
	m_pivot			= ParseXmlAttribute(visualDefElement, "pivot",		   m_pivot);
	m_renderLit		= ParseXmlAttribute(visualDefElement, "renderLit",	   m_renderLit);
	m_renderRounded = ParseXmlAttribute(visualDefElement, "renderRounded", m_renderRounded);
	m_cellCount		= ParseXmlAttribute(visualDefElement, "cellCount",	   m_cellCount);
	m_tint			= ParseXmlAttribute(visualDefElement, "tint",		   m_tint);
	billboardString = ParseXmlAttribute(visualDefElement, "billboardType", billboardString);
	shaderName	    = ParseXmlAttribute(visualDefElement, "shader",		   shaderName);
	spriteSheetName = ParseXmlAttribute(visualDefElement, "spriteSheet",   spriteSheetName);
	
	if(m_renderLit)
	{
		m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), InputLayoutType::VERTEX_PCUTBN);
	}
	else
	{
		m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), InputLayoutType::VERTEX_PCU);
	}

	m_texture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetName.c_str());

	m_spriteSheet = new SpriteSheet(*m_texture, m_cellCount);

	SetBillboardType(billboardString);

	XmlElement const* animGroupElement = visualDefElement.FirstChildElement();

	while(animGroupElement)
	{
		std::string name = animGroupElement->Name();
		GUARANTEE_OR_DIE(name == "AnimationGroup", "Failed to load Animation Group. Element Does not exist or named something other than \"AnimationGroup\"; ActorDefinition::InitializeVisuals");

		AnimationGroup animGroup = AnimationGroup(*animGroupElement, *m_spriteSheet);
		m_animGroups.push_back(animGroup);

		animGroupElement = animGroupElement->NextSiblingElement();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeAudio(XmlElement const& audioDefElement)
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
void ActorDefinition::SetFaction(std::string factionName)
{

	if(factionName == "Marine")
	{
		m_faction = Faction::MARINE;
	}
	else if (factionName == "Demon")
	{
		m_faction = Faction::DEMON;
	}
	else
	{
		m_faction = Faction::NEUTRAL;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::SetBillboardType(std::string billboardType)
{
	if(billboardType == "FullFacing")
	{
		m_billboardType = BillboardType::FULL_FACING;
	}
	else if(billboardType == "FullOpposing")
	{
		m_billboardType = BillboardType::FULL_OPPOSING;
	}
	else if(billboardType == "WorldUpFacing")
	{
		m_billboardType = BillboardType::WORLD_UP_FACING;
	}
	else if(billboardType == "WorldUpOpposing")
	{
		m_billboardType = BillboardType::WORLD_UP_OPPOSING;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("Billboard type: %s; does not exist. ActorDefinition::SetBillboardType", billboardType.c_str()));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AnimationGroup ActorDefinition::GetAnimationByName(std::string animName)
{
	for(AnimationGroup animGroup : m_animGroups)
	{
		if(animGroup.m_name == animName)
		{
			return animGroup;
		}
	}

	return AnimationGroup();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SoundID ActorDefinition::GetSoundByName(std::string soundName)
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

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AnimationGroup::AnimationGroup(XmlElement const& actorDefElement, SpriteSheet const& spriteSheet)
{	
	std::string playbackName;
	float secondsPerFrame = 0.f;
	SpriteAnimPlaybackType playbackType = SpriteAnimPlaybackType::ONCE;

	m_name = ParseXmlAttribute(actorDefElement, "name", m_name);
	m_scaleBySpeed = ParseXmlAttribute(actorDefElement, "scaleBySpeed", m_scaleBySpeed);

	secondsPerFrame = ParseXmlAttribute(actorDefElement, "secondsPerFrame", secondsPerFrame);
	playbackName = ParseXmlAttribute(actorDefElement, "playbackMode", playbackName);

	if(playbackName == "Loop")
	{
		playbackType = SpriteAnimPlaybackType::LOOP;
	}
	else if(playbackName == "Once")
	{
		playbackType = SpriteAnimPlaybackType::ONCE;
	}
	else if(playbackName == "PingPong")
	{
		playbackType = SpriteAnimPlaybackType::PINGPONG;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("Playback type: %s; does not exist. AnimationGroup::AnimationGroup", playbackName.c_str()));
	}

	XmlElement const* dirGroup = actorDefElement.FirstChildElement();

	while(dirGroup)
	{
		std::string name = dirGroup->Name();
		GUARANTEE_OR_DIE(name == "Direction", "Failed to load Direction Element. Element does not exist or named something other than \"Direction\"; AnimationGroup::AnimationGroup");

		std::string direction = ParseXmlAttribute(*dirGroup, "vector", "0.f, 0.f, 0.f");

		XmlElement const* animElement = dirGroup->FirstChildElement();

		int start = ParseXmlAttribute(*animElement, "startFrame", -1);
		int end = ParseXmlAttribute(*animElement,	"endFrame", -1);

		SpriteAnimDefinition* currentAnim = new SpriteAnimDefinition(spriteSheet, start, end, 1.f / secondsPerFrame, playbackType);

		m_animDefinitionBasedOnDirection[direction] = currentAnim;

		dirGroup = dirGroup->NextSiblingElement();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SpriteAnimDefinition* AnimationGroup::GetAnimationDefinitionBasedOnViewingDirection(Vec3 const& viewingDriection)
{

	std::string currentDirectionString = m_animDefinitionBasedOnDirection.begin()->first;
	
	Vec3 currentDirection;
	currentDirection.SetFromText(currentDirectionString.c_str());
	currentDirection.Normalize();
	
	SpriteAnimDefinition* currentDef = m_animDefinitionBasedOnDirection.begin()->second;

	float maxDot = DotProduct3D(currentDirection, viewingDriection);

	for(const auto& [key, value] : m_animDefinitionBasedOnDirection)
	{
		currentDirection.SetFromText(key.c_str());
		currentDirection.Normalize();

		float currentDot = DotProduct3D(currentDirection, viewingDriection);

		if(currentDot > maxDot)
		{
			currentDirectionString = key;
			maxDot = currentDot;
			currentDef = value;
		}
	}

	return currentDef;
}
