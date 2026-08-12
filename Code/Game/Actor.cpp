#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Game/GameCommon.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern Game* g_game;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor::Actor(Map* map, std::string const& name, Vec3 const& position, EulerAngles const& orientation, ActorHandle const& actorHandle, Actor* owner)
	: m_map(map)
	, m_position(position)
	, m_orientation(orientation)
	, m_handle(actorHandle)
	, m_owner(owner)
{
	for(size_t actorDefIndex = 0; actorDefIndex < ActorDefinition::s_actorDefinitions.size(); ++actorDefIndex)
	{
		if(ActorDefinition::s_actorDefinitions[actorDefIndex].m_name == name)
		{
			m_definition = &ActorDefinition::s_actorDefinitions[actorDefIndex];
			m_corpseTimer = Timer(m_definition->m_corpseLifetime, m_map->m_game->m_gameClock);

			break;
		}
	}
	
	if(m_definition)
	{
		m_health = m_definition->m_health;
		for(size_t weaponIndex = 0; weaponIndex < m_definition->m_weaponInventory.size(); ++weaponIndex)
		{
			std::string weaponName = m_definition->m_weaponInventory[weaponIndex];

			for(size_t index = 0; index < WeaponDefinition::s_weaponDefinitions.size(); ++index)
			{
				if(WeaponDefinition::s_weaponDefinitions[index].m_name == weaponName)
				{
					m_weaponInventory.push_back(new Weapon(&WeaponDefinition::s_weaponDefinitions[index], this));
				}
			}
		}

		if(m_weaponInventory.size() > 0)
		{
			m_currentWeapon = m_weaponInventory[0];
		}

		if(m_definition->m_aiEnabled)
		{
			m_aiController = new AIController(m_map);
			m_aiController->Possess(this);
		}

		if(m_definition->m_name != "SpawnPoint")
		{
			m_animationTimer = Timer(1.f, g_game->m_gameClock);
			SetSpawnState();
		}

		m_corpseTimer = Timer(m_definition->m_corpseLifetime, m_map->m_game->m_gameClock);

		if(m_definition->m_lifetime != -1.f)
		{
			m_lifetimeTimer = Timer(m_definition->m_lifetime, g_game->m_gameClock);
			m_lifetimeTimer.Start();
		}

		m_light = m_definition->m_light;
		m_color = m_definition->m_tint;
	}


}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor::~Actor()
{
	delete m_aiController;
	m_aiController = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::Update()
{

	if(m_possessedController && m_possessedController == m_aiController)
	{
		m_possessedController->Update();
	}

	if(m_state != ActorState::WALKING && m_state != ActorState::DYING && m_state != ActorState::DEAD && m_animationTimer.HasPeriodElapsed())
	{
		SetActorState(m_defaultState);
	}

	if(m_currentWeapon && m_currentWeapon->m_animationTimer.HasPeriodElapsed())
	{
		m_currentWeapon->SetWeaponState(WeaponState::IDLE);
	}
	
	if(m_lifetimeTimer.HasPeriodElapsed())
	{
		SetActorState(ActorState::DEAD);
	}

	if(g_game->m_isDebugMode)
	{
		Mat44 transform = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		transform.SetTranslation3D(GetEyePosition());
		DebugAddBasis(transform, 0.f, 0.25f, DebugRenderMode::USE_DEPTH);
		DebugAddWorldWireframeCylinder(m_position, m_definition->m_physicsHeight, m_definition->m_physicsRadius, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::DeathStateUpdate()
{
	if(m_state != ActorState::DYING && m_state != ActorState::DEAD)
	{
		if(m_health < 1.f)
		{
			SetActorState(ActorState::DYING);
		}
	}
	else if(m_state == ActorState::DYING)
	{
		if(m_animationTimer.HasPeriodElapsed() && m_corpseTimer.HasPeriodElapsed())
		{
			SetActorState(ActorState::DEAD);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::PhysicsUpdate()
{

	if(m_state == ActorState::DYING || m_state == ActorState::DEAD)
	{
		return;
	}

	float gravity = g_gameConfigBlackboard.GetValue("gravity", 0.0f);

	float drag;
	
	if(m_isGrounded)
	{
		drag = m_definition->m_drag;
	}
	else
	{
		drag = 0.f;
	}

	AddForce(-m_velocity * drag);

	if(!m_isGrounded && m_definition->m_effectedByGravity)
	{
		AddForce(Vec3::DOWN * gravity);
	}

	m_position += m_velocity * g_game->m_gameClock->GetDeltaSeconds();
	m_velocity += m_acceleration * g_game->m_gameClock->GetDeltaSeconds();
	m_acceleration = Vec3::ZERO;

	if(m_definition->m_isLightSource)
	{
		m_light.SetPosition(m_position);
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::Render(Camera const& camera)
{

	bool isPossessedControllerCamera = false;

	if(m_possessedController)
	{
		PlayerController* playerController = dynamic_cast<PlayerController*>(m_possessedController);

		if(playerController)
		{
			isPossessedControllerCamera = &playerController->m_worldCamera == &camera;
		}

	}

	if(m_definition && m_definition->m_name != "SpawnPoint" && !isPossessedControllerCamera)
	{
		SetAnimationIfViewChanged(camera);

		FillActorVerts();

		Mat44 targetTransform = camera.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		targetTransform.SetTranslation3D(camera.m_position);

		Mat44 billboardTransform = GetBillboardTransform(m_definition->m_billboardType, targetTransform, m_position);
		billboardTransform.SetTranslation3D(m_position);

		g_theRenderer->SetModelConstants(billboardTransform, m_color);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->BindShader(m_definition->m_shader);
		g_theRenderer->BindTexture(m_definition->m_texture);

		if(m_definition->m_renderLit)
		{
			g_theRenderer->DrawIndexedVertexArray(m_verts, m_indexes);
		}
		else
		{
			g_theRenderer->DrawVertexArray(m_unlitVerts);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::RenderDepth()
{
	if(!m_definition->m_renderLit)
	{
		return;
	}

	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->DrawIndexedVertexArray(m_verts, m_indexes);
}
	

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 Actor::GetModelMatrix() const
{

	EulerAngles yawOnly = EulerAngles(m_orientation.m_yawDegrees, 0.f, 0.f);

	Mat44 modelMatrix = yawOnly.GetAsMatrix_IFwd_JLeft_KUp();
	modelMatrix.SetTranslation3D(m_position);

	return modelMatrix;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Actor::GetForwardVector()
{	
	Mat44 orientationMat;

	orientationMat = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	
	return orientationMat.GetIBasis3D().GetNormalized();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Actor::GetUpVector()
{
	Mat44 orientationMat;

	orientationMat = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();

	return orientationMat.GetKBasis3D().GetNormalized();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Actor::GetLeftVector()
{
	Mat44 orientationMat;

	orientationMat = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();

	return orientationMat.GetJBasis3D().GetNormalized();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::EquipWeapon(int weaponIndex)
{
	if(weaponIndex < static_cast<int>(m_weaponInventory.size()))
	{
		m_currentWeapon = m_weaponInventory[weaponIndex];
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::CycleNextWeapon()
{
	int weaponIndex = 0;

	for(int index = 0; index < static_cast<int>(m_weaponInventory.size()); ++index)
	{
		if(m_weaponInventory[index] == m_currentWeapon)
		{
			weaponIndex = index;
			break;
		}
	}

	weaponIndex = (weaponIndex + 1) % static_cast<int>(m_weaponInventory.size());

	EquipWeapon(weaponIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::CyclePrevWeapon()
{

	int weaponIndex = 0;

	for(int index = 0; index < static_cast<int>(m_weaponInventory.size()); ++index)
	{
		if(m_weaponInventory[index] == m_currentWeapon)
		{
			weaponIndex = index;
			break;
		}
	}

	weaponIndex = (weaponIndex - 1);

	if(weaponIndex < 0)
	{
		weaponIndex = static_cast<int>(m_weaponInventory.size()) - 1;
	}

	EquipWeapon(weaponIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::AddToInventory(Weapon* weapon)
{
	for(size_t index = 0; index < static_cast<int>(m_weaponInventory.size()); ++index)
	{
		if(m_weaponInventory[index] == weapon)
		{
			return;
		}
	}

	m_weaponInventory.push_back(weapon);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::Attack()
{	
	m_currentWeapon->Fire();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::FillActorVerts()
{
	m_verts.clear();
	m_unlitVerts.clear();
	m_indexes.clear();

	Vec2 pivot = m_definition->m_pivot;
	Vec2 size = m_definition->m_spriteSize;
	  
	Vec2 translationValue = -pivot * size; // filthy name. change it probably

	Vec3 bottomLeft	  = Vec3(0.f, translationValue.x,		   translationValue.y);
	Vec3 bottomRight  = Vec3(0.f, translationValue.x + size.x, translationValue.y);
	Vec3 topRight	  = Vec3(0.f, translationValue.x + size.x, translationValue.y + size.y);
	Vec3 topLeft	  = Vec3(0.f, translationValue.x,		   translationValue.y + size.y);
	
	float animationTime;
	float speedScale = 1.f;

	if(m_scaleAnimationBySpeed)
	{
		speedScale = GetClampedZeroToOne((m_velocity.GetLengthXY()) / m_definition->m_runSpeed);
	}

	animationTime = static_cast<float>(m_animationTimer.GetElapsedTime()) * speedScale;

	SpriteDefinition animSpriteDef = m_actorAnimation->GetSpriteDefAtTime(animationTime);

	AABB2 UVs = animSpriteDef.GetUVs();

	if(m_definition->m_renderLit)
	{
		if(m_definition->m_renderRounded)
		{
			AddVertsForRoundedQuad3D(m_verts, m_indexes, bottomLeft, bottomRight, topRight, topLeft, m_color, UVs);
		}
		else
		{
			AddVertsForQuad3D(m_verts, m_indexes, bottomLeft, bottomRight, topRight, topLeft, m_color, UVs);
		}
	}
	else
	{
 		if(m_definition->m_renderRounded)
 		{
 			AddVertsForRoundedQuad3D_VPCU(m_unlitVerts, bottomLeft, bottomRight, topRight, topLeft, m_color, UVs);
 		}
 		else
 		{
 			AddVertsForQuad3D(m_unlitVerts, bottomLeft, bottomRight, topRight, topLeft, m_color, UVs);
 		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::MoveInDirection(Vec3 const& direction, bool didJump)
{

	if((m_state == ActorState::ATTACKING || m_state == ActorState::HURTING ) && m_animationTimer.HasPeriodElapsed())
	{
		SetActorState(ActorState::WALKING);
	}

	m_velocity.x = direction.x;
	m_velocity.y = direction.y;

	AddForce(m_velocity * m_definition->m_drag);

	if(didJump)
	{
		AddImpulse(Vec3::UP * m_definition->m_jumpHeight);
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::AddForce(Vec3 const& force)
{
	m_acceleration += force;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::TurnInDirection(float angle, float turnRate)
{
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, angle, turnRate);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::OnCollide(Actor* collidingActor)
{
	if(collidingActor)
	{
		Vec3 fwdXY = GetForwardVector().GetXY().GetNormalized();

		Vec3 impulse = fwdXY * m_definition->m_impulseOnCollide;

		collidingActor->AddImpulse(impulse * collidingActor->m_definition->m_impulseDampening);

		bool sameFaction = m_map->CheckActorAreSameFaction(collidingActor, this);
		bool notNeutral = collidingActor->m_definition->m_faction != Faction::NEUTRAL;

		if(!sameFaction && notNeutral)
		{
			collidingActor->TakeDamage(m_definition->m_damageOnCollide.GetRandomFloat(), this);
		}
	}	

	if((m_state != ActorState::DEAD && m_state != ActorState::DYING) && m_definition->m_dieOnCollide)
	{
		SetActorState(ActorState::DYING);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::OnPossessed(Controller* controller)
{
	m_possessedController = controller;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::OnUnpossessed(Controller* controller)
{
	UNUSED(controller);
	m_possessedController = m_aiController;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::TakeDamage(float damage, Actor* attackingActor)
{
	if(m_state != ActorState::DYING && m_state != ActorState::DEAD)
	{
		if(damage > 0.f)
		{
			SetActorState(ActorState::HURTING);
		}
		m_health -= damage;

		if(m_health < 1.f)
		{
			IncrementPlayerKillsOnAttackingPlayer(attackingActor);
		}

		if(m_aiController)
		{
			m_aiController->DamagedBy(attackingActor);
		}	
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::IncrementPlayerKillsOnAttackingPlayer(Actor* attackingActor)
{

	Actor* actor = nullptr;

	if(attackingActor->m_owner)
	{
		actor = attackingActor->m_owner;
	}
	else
	{
		actor = attackingActor;
	}

	PlayerController* myPlayerController = nullptr;
	PlayerController* attackingActorPlayerController = nullptr;

	if(m_possessedController && actor->m_possessedController)
	{
		myPlayerController = dynamic_cast<PlayerController*>(m_possessedController);
		attackingActorPlayerController = dynamic_cast<PlayerController*>(actor->m_possessedController);

		if(myPlayerController && attackingActorPlayerController)
		{
			attackingActorPlayerController->m_kills += 1;
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::SetAnimationIfViewChanged(Camera const& camera)
{
	AnimationGroup currentAnimation = GetAnimationGroupByState(m_state);

	m_scaleAnimationBySpeed = currentAnimation.m_scaleBySpeed;

	Vec3 viewingDirection = GetViewingDirectionFromCamera(camera);

	SpriteAnimDefinition* spriteAnimDefinition = currentAnimation.GetAnimationDefinitionBasedOnViewingDirection(Vec3(viewingDirection.x, viewingDirection.y, 0.f).GetNormalized());

	if(m_actorAnimation != spriteAnimDefinition)
	{
		m_actorAnimation = spriteAnimDefinition;

		m_animationTimer.m_period = m_actorAnimation->GetNumberOfFramesForAnimation() / m_actorAnimation->GetFPS();

		m_animationTimer.Restart();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::SetSpawnState()
{
	if(m_definition->m_name == "BloodSplatter" || m_definition->m_name == "BulletHit")
	{
		SetActorState(ActorState::DYING);
		m_defaultState = ActorState::DYING;
	}
	else
	{
		SetActorState(ActorState::WALKING);
		m_defaultState = ActorState::WALKING;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SpriteDefinition Actor::GetAnimationSpriteDef()
{	
	return m_actorAnimation->GetSpriteDefAtTime(static_cast<float>(m_animationTimer.GetElapsedTime()));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AnimationGroup Actor::GetAnimationGroupByState(ActorState state)
{
	if(state == ActorState::WALKING)
	{
		return m_definition->GetAnimationByName("Walk");
	}
	else if(state == ActorState::ATTACKING)
	{
		return m_definition->GetAnimationByName("Attack");
	}
	else if(state == ActorState::HURTING)
	{
		return m_definition->GetAnimationByName("Hurt");
	}
	else if(state == ActorState::DYING)
	{
		return m_definition->GetAnimationByName("Death");
	}

	return m_definition->GetAnimationByName("Death");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::SetActorState(ActorState newState)
{
	if(m_state != newState)
	{
		m_state = newState;

		AnimationGroup currentAnimation = GetAnimationGroupByState(m_state);

		SpriteAnimDefinition* spriteAnimDefinition = currentAnimation.GetAnimationDefinitionBasedOnViewingDirection(Vec3(1.f, 0.f, 0.f).GetNormalized());

		m_animationTimer.m_period = spriteAnimDefinition->GetNumberOfFramesForAnimation() / spriteAnimDefinition->GetFPS();

		m_animationTimer.Restart();

		SoundID newAudioID = GetSoundIDForCurrentState();

		if(newAudioID != MISSING_SOUND_ID)
		{
			if(m_currentAudioID != MISSING_SOUND_ID && g_theAudioSystem->IsPlaying(m_currentAudioID))
			{
				g_theAudioSystem->StopSound(m_currentAudioID);
			}

			m_currentAudioID = g_theAudioSystem->StartSoundAt(newAudioID, m_position, false, 1.f);

		}

	}

	if(newState == ActorState::DYING)
	{
		if(m_corpseTimer.IsStopped())
		{
			m_corpseTimer.Start();
		}
	}

	if(newState == ActorState::DEAD)
	{
		PlayerController* playerController = nullptr;

		if(m_possessedController)
		{
			playerController = dynamic_cast<PlayerController*>(m_possessedController);
			
			if(playerController)
			{
				playerController->m_deaths += 1;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Actor::GetViewingDirectionFromCamera(Camera const& camera)
{
	Vec3 cameraToActor = m_position - camera.m_position;

	Mat44 worldToModelTransform = GetModelMatrix().GetOrthonormalInverse(); 
	
	Vec3 localXY = worldToModelTransform.TransformVectorQuantity3D(cameraToActor);

	return localXY.GetNormalized();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SoundID Actor::GetSoundIDForCurrentState()
{
	switch(m_state)
	{
		case ActorState::WALKING:	return m_definition->GetSoundByName("Walk"); 

		case ActorState::ATTACKING: return m_definition->GetSoundByName("Attack");
		
		case ActorState::HURTING:	return m_definition->GetSoundByName("Hurt");
		
		case ActorState::DYING:     return m_definition->GetSoundByName("Death");
		
		default:					return MISSING_SOUND_ID;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Actor::UpdateSoundPosition()
{

	if(m_currentAudioID != MISSING_SOUND_ID)
	{
		if(g_theAudioSystem->IsPlaying(m_currentAudioID))
		{
			g_theAudioSystem->SetSoundPosition(m_currentAudioID, m_position);
		}
		else
		{
			m_currentAudioID = static_cast<SoundPlaybackID>(-1);
		}
	}

	if(m_currentWeapon && m_currentWeapon->m_currentAudioID != MISSING_SOUND_ID)
	{
		if(g_theAudioSystem->IsPlaying(m_currentWeapon->m_currentAudioID))
		{
			g_theAudioSystem->SetSoundPosition(m_currentWeapon->m_currentAudioID, m_position);
		}
		else
		{
			m_currentAudioID = static_cast<SoundPlaybackID>(-1);
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Actor::IsAlive()
{
	return (m_state == ActorState::WALKING && m_definition->m_name != "PlasmaProjectile");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Actor::GetEyePosition()
{
	return m_position + Vec3(0.f, 0.f, m_definition->m_eyeHeight);
}


