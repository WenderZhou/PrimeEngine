#ifndef _CHARACTER_CONTROL_TARGET_
#define _CHARACTER_CONTROL_TARGET_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Events/StandardEvents.h"

namespace CharacterControl 
{
namespace Events 
{
struct Event_CREATE_TARGET : public PE::Events::Event_CREATE_MESH
{
	PE_DECLARE_CLASS(Event_CREATE_TARGET);

	Event_CREATE_TARGET(int& threadOwnershipMask) : PE::Events::Event_CREATE_MESH(threadOwnershipMask) {}

	// override SetLuaFunctions() since we are adding custom Lua interface
	static void SetLuaFunctions(PE::Components::LuaEnvironment* pLuaEnv, lua_State* luaVM);

	// Lua interface prefixed with l_
	static int l_Construct(lua_State* luaVM);

	char m_patrolWayPoint[32];
};
}

namespace Components {

struct Target : public PE::Components::Component
{
	PE_DECLARE_CLASS(Target);

	Target(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_TARGET* pEvt);

	virtual void addDefaultComponents();

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
	virtual void do_UPDATE(PE::Events::Event* pEvt);

	char m_patrolWayPoint[32];
};
}; // namespace Components
}; // namespace CharacterControl
#endif
