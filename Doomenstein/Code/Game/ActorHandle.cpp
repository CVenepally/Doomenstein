#include "Game/ActorHandle.hpp"

const ActorHandle ActorHandle::INVALID = ActorHandle(0x0000FFFF, 0x0000FFFF);

// 0x4uid4ind


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ActorHandle::ActorHandle(unsigned int uid, unsigned int index)
{
	unsigned int bitMask = 0x0000ffff;

	unsigned int maskedIndex = index & bitMask;
	unsigned int leftShiftedUID = uid << 16;

	m_data = maskedIndex | leftShiftedUID;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ActorHandle::IsValid() const
{
	return (*this != ActorHandle::INVALID);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int ActorHandle::GetIndex() const
{
	unsigned int index = m_data & 0x0000ffff;

	return index;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ActorHandle::operator==(ActorHandle const& other) const
{
	return m_data == other.m_data;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ActorHandle::operator!=(ActorHandle const& other) const
{
	return m_data != other.m_data;
}