#include "PhysicsManager.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"

namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(Collider, Component);
PE_IMPLEMENT_CLASS1(PhysicsManager, Component);

Collider::Collider(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself) : Component(context, arena, hMyself)
{

}

PhysicsManager::PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself) : Component(context, arena, hMyself), m_colliders(context, arena, 1)
{

}

void PhysicsManager::addDefaultComponents()
{
	Component::addDefaultComponents();

	// Data components

	// event handlers
	PE_REGISTER_EVENT_HANDLER(Events::Event_PHYSICS_START, PhysicsManager::do_PHYSICS_START);
	PE_REGISTER_EVENT_HANDLER(Events::Event_PRE_RENDER_needsRC, PhysicsManager::do_PRE_RENDER_needsRC);
}

Handle PhysicsManager::addCollider()
{
	Handle hCollider("Collider", sizeof(Collider));
	Collider* pCollider = new(hCollider) Collider(*m_pContext, m_arena, hCollider);
	m_colliders.add(hCollider);
	return hCollider;
}

void PhysicsManager::do_PHYSICS_START(Events::Event* pEvt)
{
	for (int i = 0; i < m_colliders.m_size; i++)
	{
		Collider* pCollider_i = m_colliders[i].getObject<Collider>();
		for (int j = i + 1; j < m_colliders.m_size; j++)
		{
			Collider* pCollider_j = m_colliders[j].getObject<Collider>();
			if (pCollider_i->m_type == Collider::STATIC && pCollider_j->m_type == Collider::STATIC)
				continue;
			else if (pCollider_i->m_type == Collider::DYNAMIC && pCollider_j->m_type == Collider::DYNAMIC)
			{
				continue;
			}
			else
			{
				Collider* staticCollider = pCollider_i->m_type == Collider::STATIC ? pCollider_i : pCollider_j;
				Collider* dynamicCollider = pCollider_i->m_type == Collider::STATIC ? pCollider_j : pCollider_i;

				float d[6];
				for (int k = 0; k < 6; k++)
					d[k] = dynamicCollider->position * staticCollider->normals[k] + staticCollider->d[k];

				// test if collided
				if (d[0] < dynamicCollider->radius && d[1] < dynamicCollider->radius &&
					d[2] < dynamicCollider->radius && d[3] < dynamicCollider->radius &&
					d[4] < dynamicCollider->radius && d[5] < dynamicCollider->radius)
				{
					for (int k = 0; k < 6; k++)
					{
						if (d[k] > 0)
						{
							dynamicCollider->position += (dynamicCollider->radius - d[k]) * staticCollider->normals[k];
						}
					}

					dynamicCollider->decendingSpeed = 0.0f;
				}
			}
		}
	}
}

void PhysicsManager::do_PRE_RENDER_needsRC(Events::Event* pEvt)
{
	/*Vector3 red(1.0f, 0.0f, 0);
	Vector3 green(0.0f, 1.0f, 0.0f);
	Vector3 blue(0.0f, 0.0f, 1.0f);

	for (int i = 0; i < m_colliders.m_size; i++)
	{
		Collider* pCollider = m_colliders[i].getObject<Collider>();
		if (pCollider->m_type == Collider::STATIC)
		{
			Vector3 linepts[] =
			{
				pCollider->vertices[0], red, pCollider->vertices[2], red,
				pCollider->vertices[0], green, pCollider->vertices[4], green,
			};

			Matrix4x4 base;

			DebugRenderer::Instance()->createLineMesh(false, base, &linepts[0].m_x, 4, 0);
		}
	}*/
}

}; // namespace Components
}; // namespace PE