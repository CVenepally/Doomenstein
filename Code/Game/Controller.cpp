#include "Game/Controller.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Controller::Controller(Map* map)
	: m_map(map)
{
	m_actorHandle = ActorHandle::INVALID;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Controller::~Controller()
{
	m_map = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Controller::Possess(Actor* actorToPossess)
{
	if(!actorToPossess)
	{
		return;
	}

	// unpossess current actor
	Unpossess();

	// possess
	m_actorHandle = actorToPossess->m_handle;

	actorToPossess->OnPossessed(this);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Controller::Unpossess()
{
	if(m_actorHandle != ActorHandle::INVALID)
	{
		Actor* currentActor = GetActor();
		if(currentActor)
		{
			currentActor->OnUnpossessed(this);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Actor* Controller::GetActor() const
{
	return m_map->GetActorByHandle(m_actorHandle);
}
