#pragma once
#include "Game/Controller.hpp"
#include "Game/ActorHandle.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Actor;
class Map;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class AIController : public Controller
{

public:
	AIController() = default;
	AIController(Map* map);

	~AIController();

	virtual void Update() override;
	void LookAndMoveTowardsActor(Actor* targetActor, Actor* controlledActor);
	void AttackActor(Actor* controlledActor);
	void DamagedBy(Actor* attackActor);

public:

	ActorHandle m_targetActorHandle;

};