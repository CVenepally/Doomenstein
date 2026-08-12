#include "Game/AIController.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AIController::AIController(Map* map)
	:Controller(map)
{
	m_targetActorHandle = ActorHandle::INVALID;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
AIController::~AIController()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AIController::Update()
{

	if(!m_map->m_game->m_aiEnabled)
	{
		return;
	}

	Actor* targetActor = m_map->GetActorByHandle(m_targetActorHandle);
	Actor* controlledActor = GetActor();

	if(controlledActor && controlledActor->m_state != ActorState::DYING && controlledActor->m_state != ActorState::DEAD)
	{
		if(!targetActor)
		{
			targetActor = m_map->GetClosestVisibleActor(controlledActor);

			if(targetActor)
			{
				m_targetActorHandle = targetActor->m_handle;
			}
		}

		if(targetActor)
		{
			LookAndMoveTowardsActor(targetActor, controlledActor);
		}
	}	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AIController::LookAndMoveTowardsActor(Actor* targetActor, Actor* controlledActor)
{
	Vec3 posToTargetPos = (targetActor->m_position - controlledActor->m_position);

	float angle = posToTargetPos.GetAngleAboutZDegrees();

	controlledActor->TurnInDirection(angle, controlledActor->m_definition->m_turnSpeed * m_map->m_game->m_gameClock->GetDeltaSeconds());

	float distanceBetweenActors = GetVectorDistance3D(targetActor->m_position, controlledActor->m_position);
	float actorRadius = controlledActor->m_definition->m_physicsRadius;
	float targetActorRadius = targetActor->m_definition->m_physicsRadius;
	float tolerance = 0.4f;

	if(distanceBetweenActors > targetActorRadius + actorRadius + tolerance)
	{
		controlledActor->MoveInDirection(controlledActor->GetForwardVector().GetXY().GetNormalized() * controlledActor->m_definition->m_runSpeed);
	}
	else
	{
		AttackActor(controlledActor);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AIController::AttackActor(Actor* controlledActor)
{
	controlledActor->Attack();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AIController::DamagedBy(Actor* attackingActor)
{	
	if(attackingActor)
	{
		if(attackingActor->m_definition->m_name == "PlasmaProjectile" && attackingActor->m_owner)
		{
			m_targetActorHandle = attackingActor->m_owner->m_handle;
			return;
		}

		m_targetActorHandle = attackingActor->m_handle;
	}
}
