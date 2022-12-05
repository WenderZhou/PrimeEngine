#ifndef __PYENGINE_2_0_PHYSICS_MANAGER_H__
#define __PYENGINE_2_0_PHYSICS_MANAGER_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "../Events/Component.h"
#include "../Events/Event.h"
#include "../Events/StandardEvents.h"
#include "../Utils/Array/Array.h"

// Sibling/Children includes
namespace PE {
namespace Components {

struct Collider : public Component
{
	PE_DECLARE_CLASS(Collider);

	typedef enum { STATIC, DYNAMIC } ColliderType;

	ColliderType m_type;

	// Static Collider Bounding Box
	Vector3 vertices[8];
	Vector3 normals[6];
	float d[6];

	// Dynamic Collider Bounding Sphere
	Vector3 position;
	float radius;

	// gravity
	float gravity;
	float decendingSpeed;

	Collider(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);
};

struct PhysicsManager : public Component
{
	PE_DECLARE_CLASS(PhysicsManager);

	// Constructor -------------------------------------------------------------
	PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);
	virtual ~PhysicsManager() {}

	// Component ---------------------------------------------------------------
	virtual void addDefaultComponents();

	// Individual events -------------------------------------------------------
	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PHYSICS_START);
	virtual void do_PHYSICS_START(Events::Event* pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_RENDER_needsRC)
	virtual void do_PRE_RENDER_needsRC(Events::Event* pEvt);

	Handle addCollider();

	Array<Handle, 1> m_colliders;
};

}; // namespace Components
}; // namespace PE
#endif
