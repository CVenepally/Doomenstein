#include "Game/Weapon.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"
#include "Game/AIController.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefintion.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"

#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern Game* g_game;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Weapon::Weapon(WeaponDefinition* weaponDefinition, Actor* owner)
	: m_weaponDefinition(weaponDefinition)
	, m_owner(owner)
{
	m_refireTimer = Timer(m_weaponDefinition->m_refireTime, g_game->m_gameClock);
	m_animationTimer = Timer(0.0f, g_game->m_gameClock);
	m_animationTimer.Start();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Weapon::~Weapon()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::Fire()
{

	if(m_refireTimer.IsStopped())
	{
		m_refireTimer.Start();

		if(m_weaponDefinition->m_name == "Pistol")
		{
			for(int count = 0; count < m_weaponDefinition->m_rayCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);
				FirePistol();
			}

			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}

		if(m_weaponDefinition->m_name == "PlasmaRifle")
		{
			for(int count = 0; count < m_weaponDefinition->m_projectileCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);
				FirePlasma();
			}

			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}

		if(m_weaponDefinition->m_name == "DemonMelee" || m_weaponDefinition->m_name == "HeavyDemonMelee")
		{
			for(int count = 0; count < m_weaponDefinition->m_meleeCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);

				Melee();
			}
			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}
	}

	// replace this if with while for spam fire

	if(m_refireTimer.HasPeriodElapsed())
	{
		if(m_weaponDefinition->m_name == "Pistol")
		{
			for(int count = 0; count < m_weaponDefinition->m_rayCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);
				FirePistol();
			}

			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}

		if(m_weaponDefinition->m_name == "PlasmaRifle")
		{
			for(int count = 0; count < m_weaponDefinition->m_projectileCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);
				FirePlasma();
			}

			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}

		if(m_weaponDefinition->m_name == "DemonMelee" || m_weaponDefinition->m_name == "HeavyDemonMelee")
		{
			for(int count = 0; count < m_weaponDefinition->m_meleeCount; ++count)
			{
				m_owner->SetActorState(ActorState::ATTACKING);
				SetWeaponState(WeaponState::ATTACK);
				Melee();
			}

			SoundID newAudioID = GetSoundIDForCurrentState();

			if(newAudioID != MISSING_SOUND_ID)
			{
				if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
				{
					g_theAudioSystem->StopSound(m_currentAudioID);
				}
				m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_owner->m_position, false, 1.f);
			}
		}

		// remove these for spam fire
		m_refireTimer.Stop();
		m_refireTimer.Start();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::FirePistol()
{

	Vec3 start = (m_owner->GetEyePosition() - m_owner->GetUpVector() * 0.03f) + m_owner->GetForwardVector() * m_owner->m_definition->m_physicsRadius * 1.01f;

	RaycastResult shotResult = m_owner->m_map->RaycastAll(start, GetRandomDirectionInCone(m_weaponDefinition->m_rayCone), m_weaponDefinition->m_rayRange, m_owner);

	if(shotResult.m_hitActor)
	{
		shotResult.m_rayResult.m_rayForwardNormal = Vec3(shotResult.m_rayResult.m_rayForwardNormal.x, shotResult.m_rayResult.m_rayForwardNormal.y, 0.f).GetNormalized();

		shotResult.m_hitActor->AddImpulse(shotResult.m_rayResult.m_rayForwardNormal * m_weaponDefinition->m_rayImpulse);
		shotResult.m_hitActor->TakeDamage(m_weaponDefinition->m_rayDamage.GetRandomFloat(), m_owner);

		SpawnInfo bloodSpawnInfo;
		bloodSpawnInfo.m_actorName = "BloodSplatter";
		bloodSpawnInfo.m_position = shotResult.m_rayResult.m_impactPos;

		Actor* bloodActor = m_owner->m_map->SpawnActor(bloodSpawnInfo);
		bloodActor->SetActorState(ActorState::DYING);
	}
	else
	{
		SpawnInfo bulletHitSpawnInfo;
		bulletHitSpawnInfo.m_actorName = "BulletHit";
		bulletHitSpawnInfo.m_position = shotResult.m_rayResult.m_impactPos;

		Actor* bloodActor = m_owner->m_map->SpawnActor(bulletHitSpawnInfo);
		bloodActor->SetActorState(ActorState::DYING);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::FirePlasma()
{

	float bottomOffSet = 0.09f;
	float forwardOffSet = m_owner->m_definition->m_physicsRadius * 1.5f;
	SpawnInfo spawnInfo;
	spawnInfo.m_actorName = "PlasmaProjectile";
	spawnInfo.m_orientation = m_owner->m_orientation;
	spawnInfo.m_position = (m_owner->GetEyePosition() - m_owner->GetUpVector() * bottomOffSet) + m_owner->GetForwardVector() * forwardOffSet;
	spawnInfo.m_velocity = GetRandomDirectionInCone(m_weaponDefinition->m_projectileCone) * m_weaponDefinition->m_projectileSpeed;

	Actor* projectile = m_owner->m_map->SpawnActor(spawnInfo);
	projectile->m_owner = m_owner;
	projectile->SetActorState(ActorState::WALKING);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::Melee()
{
	if(m_owner)
	{
		for(Actor* actor : m_owner->m_map->m_allActors)
		{
			if(actor && actor->m_definition->m_faction != m_owner->m_definition->m_faction && actor->m_definition->m_faction != Faction::NEUTRAL && actor->m_definition->m_name != "PlasmaProjectile")
			{
				Cylinder3D actorCylinder = Cylinder3D(actor->m_position, actor->m_definition->m_physicsHeight, actor->m_definition->m_physicsRadius);

				Vec2 startPoint = actor->m_position.GetXY2D();

				float meleeRange = m_weaponDefinition->m_meleeRange;
				float meleeArc = m_weaponDefinition->m_meleeArc;
				float meleeDamage = m_weaponDefinition->m_meleeDamage.GetRandomFloat();

				Vec3 attackStartPoint = m_owner->GetEyePosition() + m_owner->GetForwardVector() * m_owner->m_definition->m_physicsRadius;
				Vec2 sectorStartPoint = attackStartPoint.GetXY2D();

				if(IsPointInsideDirectedSector2D(startPoint, sectorStartPoint, m_owner->GetForwardVector().GetXY2D().GetNormalized(), meleeArc, meleeRange))
				{
					float impulseMag = m_weaponDefinition->m_meleeImpulse;
					Vec3 impulse = m_owner->GetForwardVector().GetXY().GetNormalized() * impulseMag;

					actor->TakeDamage(meleeDamage, m_owner);
					actor->AddImpulse(impulse);
				}
			}	
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::Render(Camera const& camera)
{
	if(m_weaponDefinition->m_name == "DemonMelee" || m_weaponDefinition->m_name == "HeavyDemonMelee")
	{
		return;
	}

	RenderHUD(camera);
	RenderReticle(camera);
	RenderWeapon(camera);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::RenderHUD(Camera const& camera) const
{
	std::vector<Vertex_PCU> hudVerts;

	float viewportHeight = camera.m_viewportBounds.m_maxs.y - camera.m_viewportBounds.m_mins.y;

	AABB2 hudAABB;
	hudAABB.m_mins = camera.m_viewportBounds.m_mins;
	hudAABB.m_maxs = Vec2(camera.m_viewportBounds.m_maxs.x, camera.m_viewportBounds.m_mins.y + (viewportHeight * 0.15f));
	AddVertsForAABB2D(hudVerts, hudAABB, Rgba8::WHITE, AABB2::ZERO_TO_ONE);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(m_weaponDefinition->m_shader);
	g_theRenderer->BindTexture(m_weaponDefinition->m_hudTexture);
	g_theRenderer->DrawVertexArray(hudVerts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::RenderReticle(Camera const& camera) const
{
	std::vector<Vertex_PCU> reticleVerts;

	AABB2 reticleAABB = AABB2(Vec2::ZERO, m_weaponDefinition->m_reticleSize);
	reticleAABB.SetCenter(camera.m_viewportBounds.GetCenter());

	AddVertsForAABB2D(reticleVerts, reticleAABB, Rgba8::WHITE, AABB2::ZERO_TO_ONE);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(m_weaponDefinition->m_shader);
	g_theRenderer->BindTexture(m_weaponDefinition->m_reticleTexture);
	g_theRenderer->DrawVertexArray(reticleVerts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::RenderWeapon(Camera const& camera)
{
	std::vector<Vertex_PCU> weaponVerts;

	float viewportHeight = camera.m_viewportBounds.m_maxs.y - camera.m_viewportBounds.m_mins.y;
	float spriteScale = 1.f / m_owner->m_map->m_game->GetNumPlayerControllers();

	AABB2 weaponAABB = AABB2(Vec2::ZERO, m_weaponDefinition->m_spriteSize * spriteScale);
	weaponAABB.SetCenter(Vec2(camera.m_viewportBounds.m_maxs.x * m_weaponDefinition->m_pivot.x, camera.m_viewportBounds.m_mins.y + viewportHeight * 0.30f));

	SpriteAnimDefinition* animDef = GetSpriteAnimDefByState();

	SpriteDefinition spriteDef = animDef->GetSpriteDefAtTime(static_cast<float>(m_animationTimer.GetElapsedTime()));

	AddVertsForAABB2D(weaponVerts, weaponAABB, Rgba8::WHITE, spriteDef.GetUVs());

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(m_weaponDefinition->m_shader);
	g_theRenderer->BindTexture(m_weaponDefinition->m_spriteSheetTexture);
	g_theRenderer->DrawVertexArray(weaponVerts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SpriteAnimDefinition* Weapon::GetSpriteAnimDefByState()
{
	if(m_state == WeaponState::ATTACK)
	{
		return m_weaponDefinition->GetAnimationSpriteDefByName("Attack");
	}
	else
	{
		return m_weaponDefinition->GetAnimationSpriteDefByName("Idle");
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SoundID Weapon::GetSoundIDForCurrentState()
{
	switch(m_state)
	{
		case IDLE: return m_weaponDefinition->GetSoundIDByName("Idle");

		case ATTACK: return m_weaponDefinition->GetSoundIDByName("Fire");
		
		default: return MISSING_SOUND_ID;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Weapon::GetRandomDirectionInCone(float offset)
{
	FloatRange randAngle = FloatRange(-offset, offset);

	Mat44 rotationMatrix = Mat44::MakeZRotationDegrees(randAngle.GetRandomFloat());
	rotationMatrix.AppendYRotation(randAngle.GetRandomFloat());

	Vec3 bulletFwd = rotationMatrix.TransformVectorQuantity3D(m_owner->GetForwardVector());

	return bulletFwd;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::SetAnimationTimerByState()
{
	if(m_weaponDefinition->m_name == "DemonMelee" || m_weaponDefinition->m_name == "HeavyDemonMelee")
	{
		return;
	}

	SpriteAnimDefinition* spriteAnimDef = GetSpriteAnimDefByState();

	float period = spriteAnimDef->GetNumberOfFramesForAnimation() / spriteAnimDef->GetFPS();
		
	m_animationTimer.m_period = period;
	m_animationTimer.Restart();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Weapon::SetWeaponState(WeaponState newState)
{
	if(m_weaponDefinition->m_name == "DemonMelee" || m_weaponDefinition->m_name == "HeavyDemonMelee")
	{
		return;
	}
	
	m_state = newState;
	SetAnimationTimerByState();
}
