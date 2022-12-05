#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Lua/EventGlue/EventDataCreators.h"

#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Target.h"
#include "ClientGameObjectManagerAddon.h"
#include "CharacterControlContext.h"

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl {
namespace Events {

PE_IMPLEMENT_CLASS1(Event_CREATE_TARGET, PE::Events::Event_CREATE_MESH);

void Event_CREATE_TARGET::SetLuaFunctions(PE::Components::LuaEnvironment* pLuaEnv, lua_State* luaVM)
{
	static const struct luaL_Reg l_Event_CREATE_TARGET[] = {
		{"Construct", l_Construct},
		{NULL, NULL} // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateSoldierNPC
	luaL_register(luaVM, 0, l_Event_CREATE_TARGET);
}

int Event_CREATE_TARGET::l_Construct(lua_State* luaVM)
{
	PE::Handle h("EVENT", sizeof(Event_CREATE_TARGET));

	// get arguments from stack
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 17;

	PE::GameContext* pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));

	Event_CREATE_TARGET* pEvt = new(h) Event_CREATE_TARGET(pContext->m_gameThreadThreadOwnershipMask);

	const char* name = lua_tostring(luaVM, -numArgs--);
	const char* package = lua_tostring(luaVM, -numArgs--);

	float positionFactor = 1.0f / 100.0f;
	Vector3 pos, u, v, n;
	pos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);

	const char* wayPointName = NULL;
	if (!lua_isnil(luaVM, -numArgs))
		wayPointName = lua_tostring(luaVM, -numArgs--); // have patrol waypoint name
	else
		numArgs--; // ignore

	// set data values before popping memory off stack
	StringOps::writeToString(name, pEvt->m_meshFilename, 255);
	StringOps::writeToString(package, pEvt->m_package, 255);
	StringOps::writeToString(wayPointName, pEvt->m_patrolWayPoint, 32);

	lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

	pEvt->hasCustomOrientation = true;
	pEvt->m_pos = pos;
	pEvt->m_u = u;
	pEvt->m_v = v;
	pEvt->m_n = n;

	LuaGlue::pushTableBuiltFromHandle(luaVM, h);

	return 1;
}
};
namespace Components {

PE_IMPLEMENT_CLASS1(Target, Component);

// create waypoint form creation event
Target::Target(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_TARGET* pEvt)
	: Component(context, arena, hMyself)
{
	// need to acquire redner context for this code to execute thread-safe
	m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);

	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance* pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
	pMeshInstance->addDefaultComponents();

	pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);

	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode* pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	pSN->addComponent(hMeshInstance);

	RootSceneNode::Instance()->addComponent(hSN);

	{
		static int allowedEvts[] = { 0 };
		addComponent(hSN, &allowedEvts[0]);
	}

	pSN->m_base.setPos(pEvt->m_pos);
	pSN->m_base.setU(pEvt->m_u);
	pSN->m_base.setV(pEvt->m_v);
	pSN->m_base.setN(pEvt->m_n);

	StringOps::writeToString(pEvt->m_patrolWayPoint, m_patrolWayPoint, 32);

	m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);
}

void Target::addDefaultComponents()
{
	Component::addDefaultComponents();

	// custom methods of this component
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, Target::do_UPDATE)
}

void Target::do_UPDATE(PE::Events::Event* pEvt)
{	
	PE::Handle hTarget = getHandle();
	if (hTarget.isValid())
	{
		// see if parent has scene node component
		SceneNode* pSN = hTarget.getObject<Component>()->getFirstComponent<SceneNode>();
		if (pSN)
		{
			PE::Events::Event_UPDATE* pRealEvt = (PE::Events::Event_UPDATE*)(pEvt);

			ClientGameObjectManagerAddon* pGameObjectManagerAddon = (ClientGameObjectManagerAddon*)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
			if (pGameObjectManagerAddon)
			{
				// search for waypoint object
				WayPoint* pWP = pGameObjectManagerAddon->getWayPoint(m_patrolWayPoint);
				if (pWP)
				{
					Vector3 curPos = pSN->m_base.getPos();
					Vector3 targetPosition = pWP->m_base.getPos();

					float dsqr = (targetPosition - curPos).lengthSqr();

					if (dsqr > 0.01f)
					{
						// not at the spot yet
						static float speed = 1.4f;
						float allowedDisp = speed * pRealEvt->m_frameTime;

						Vector3 dir = targetPosition - curPos;
						dir.normalize();
						float dist = sqrt(dsqr);
						if (dist > allowedDisp)
							dist = allowedDisp; // can move up to allowedDisp

						// instantaneous turn
						pSN->m_base.turnInDirection(-dir, 3.1415f);	// I don't know why here need a negative sign
						pSN->m_base.setPos(curPos + dir * dist);
					}
					else if(pWP->m_nextWayPointCnt > 0)
					{
						StringOps::writeToString(pWP->m_nextWayPointName[rand() % pWP->m_nextWayPointCnt], m_patrolWayPoint, 32);
					}
				}
			}
		}
	}
}

}; // namespace Components
}; // namespace CharacterControl
