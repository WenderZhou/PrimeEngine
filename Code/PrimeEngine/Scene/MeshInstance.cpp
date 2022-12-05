#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/Scene/Skeleton.h"
#include "DefaultAnimationSM.h"
#include "Light.h"

#include "PrimeEngine/GameObjectModel/Camera.h"

// Sibling/Children includes
#include "MeshInstance.h"
#include "MeshManager.h"
#include "SceneNode.h"
#include "CameraManager.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "DebugRenderer.h"

using namespace PE::Events;

namespace PE {
namespace Components{

PE_IMPLEMENT_CLASS1(MeshInstance, Component);

MeshInstance::MeshInstance(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
: Component(context, arena, hMyself)
, m_culledOut(false)
{
	
}

void MeshInstance::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Event_PRE_RENDER_needsRC, MeshInstance::do_PRE_RENDER_needsRC);
}

void MeshInstance::initFromFile(const char *assetName, const char *assetPackage,
		int &threadOwnershipMask)
{
	Handle h = m_pContext->getMeshManager()->getAsset(assetName, assetPackage, threadOwnershipMask);

	initFromRegisteredAsset(h);
}

void MeshInstance::do_PRE_RENDER_needsRC(PE::Events::Event* pEvt)
{
	Event_PRE_RENDER_needsRC* pRealEvent = (Event_PRE_RENDER_needsRC*)(pEvt);
	Mesh* pMesh = m_hAsset.getObject<Mesh>();

#if 0
	// draw bounding volume
	Handle hSN = getFirstParentByType<SceneNode>();

	if (!hSN.isValid())	// NOTE: maybe something related to un-static mesh
		return;

	Matrix4x4 base = hSN.getObject<PE::Components::SceneNode>()->m_worldTransform;

	Vector3 color(1.0f, 1.0f, 0);
	Vector3 linepts[] =
	{
		base * pMesh->m_boundingVolume.vertex[0], color, base * pMesh->m_boundingVolume.vertex[1], color,
		base * pMesh->m_boundingVolume.vertex[1], color, base * pMesh->m_boundingVolume.vertex[3], color,
		base * pMesh->m_boundingVolume.vertex[3], color, base * pMesh->m_boundingVolume.vertex[2], color,
		base * pMesh->m_boundingVolume.vertex[2], color, base * pMesh->m_boundingVolume.vertex[0], color,
		base * pMesh->m_boundingVolume.vertex[4], color, base * pMesh->m_boundingVolume.vertex[5], color,
		base * pMesh->m_boundingVolume.vertex[5], color, base * pMesh->m_boundingVolume.vertex[7], color,
		base * pMesh->m_boundingVolume.vertex[7], color, base * pMesh->m_boundingVolume.vertex[6], color,
		base * pMesh->m_boundingVolume.vertex[6], color, base * pMesh->m_boundingVolume.vertex[4], color,
		base * pMesh->m_boundingVolume.vertex[0], color, base * pMesh->m_boundingVolume.vertex[4], color,
		base * pMesh->m_boundingVolume.vertex[1], color, base * pMesh->m_boundingVolume.vertex[5], color,
		base * pMesh->m_boundingVolume.vertex[3], color, base * pMesh->m_boundingVolume.vertex[7], color,
		base * pMesh->m_boundingVolume.vertex[2], color, base * pMesh->m_boundingVolume.vertex[6], color
	};

	DebugRenderer::Instance()->createLineMesh(false, base, &linepts[0].m_x, 24, 0);
#endif
}

bool MeshInstance::hasSkinWeights()
{
	Mesh *pMesh = m_hAsset.getObject<Mesh>();
	return pMesh->m_hSkinWeightsCPU.isValid();
}

void MeshInstance::initFromRegisteredAsset(const PE::Handle &h)
{
	m_hAsset = h;
	//add this instance as child to Mesh so that skin knows what to draw
	static int allowedEvts[] = {0};
	m_hAsset.getObject<Component>()->addComponent(m_hMyself, &allowedEvts[0]);
}


}; // namespace Components
}; // namespace PE
