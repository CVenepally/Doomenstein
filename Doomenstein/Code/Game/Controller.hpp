#pragma once

#include "Game/ActorHandle.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Map;
class Actor;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Controller
{
public:

	Controller(Map* map);
	~Controller();

	virtual void Update() = 0;

	void Possess(Actor* actorToPossess);
	void Unpossess();
	Actor* GetActor() const;

public:

	ActorHandle m_actorHandle;
	Map* m_map = nullptr;
};