// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "MeshManager.h"
// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/Geometry/SkeletonCPU/SkeletonCPU.h"

#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Light.h"

// Sibling/Children includes

#include "MeshInstance.h"
#include "Skeleton.h"
#include "SceneNode.h"
#include "DrawList.h"
#include "SH_DRAW.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

namespace PE {
namespace Components{

PE_IMPLEMENT_CLASS1(MeshManager, Component);
MeshManager::MeshManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
	: Component(context, arena, hMyself)
	, m_assets(context, arena, 256)
{
}

PE::Handle MeshManager::getAsset(const char *asset, const char *package, int &threadOwnershipMask)
{
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);
	
	int index = m_assets.findIndex(key);
	if (index != -1)
	{
		return m_assets.m_pairs[index].m_value;
	}
	Handle h;

	if (StringOps::endswith(asset, "skela"))
	{
		PE::Handle hSkeleton("Skeleton", sizeof(Skeleton));
		Skeleton *pSkeleton = new(hSkeleton) Skeleton(*m_pContext, m_arena, hSkeleton);
		pSkeleton->addDefaultComponents();

		pSkeleton->initFromFiles(asset, package, threadOwnershipMask);
		h = hSkeleton;
	}
	else if (StringOps::endswith(asset, "mesha"))
	{
		MeshCPU mcpu(*m_pContext, m_arena);
		mcpu.ReadMesh(asset, package, "");
		
		PE::Handle hMesh("Mesh", sizeof(Mesh));
		Mesh *pMesh = new(hMesh) Mesh(*m_pContext, m_arena, hMesh);
		pMesh->addDefaultComponents();

		pMesh->loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

#if PE_API_IS_D3D11
		// todo: work out how lods will work
		//scpu.buildLod();
#endif
        // generate collision volume here. or you could generate it in MeshCPU::ReadMesh()
        pMesh->m_performBoundingVolumeCulling = true; // will now perform tests for this mesh

		// construct the bounding volume
		float xMin = FLT_MAX, yMin = FLT_MAX, zMin = FLT_MAX;
		float xMax = FLT_MIN, yMax = FLT_MIN, zMax = FLT_MIN;

		PositionBufferCPU* pvbcpu = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>();
		for (int i = 0; i < pvbcpu->m_values.m_size / 3; i++)
		{
			xMax = max(xMax, pvbcpu->m_values[3 * i]);
			yMax = max(yMax, pvbcpu->m_values[3 * i + 1]);
			zMax = max(zMax, pvbcpu->m_values[3 * i + 2]);

			xMin = min(xMin, pvbcpu->m_values[3 * i]);
			yMin = min(yMin, pvbcpu->m_values[3 * i + 1]);
			zMin = min(zMin, pvbcpu->m_values[3 * i + 2]);
		}

		pMesh->m_boundingVolume.vertices[0] = Vector3(xMin - 1e-5, yMin - 1e-5, zMin - 1e-5);
		pMesh->m_boundingVolume.vertices[1] = Vector3(xMax + 1e-5, yMin - 1e-5, zMin - 1e-5);
		pMesh->m_boundingVolume.vertices[2] = Vector3(xMin - 1e-5, yMin - 1e-5, zMax + 1e-5);
		pMesh->m_boundingVolume.vertices[3] = Vector3(xMax + 1e-5, yMin - 1e-5, zMax + 1e-5);
		pMesh->m_boundingVolume.vertices[4] = Vector3(xMin - 1e-5, yMax + 1e-5, zMin - 1e-5);
		pMesh->m_boundingVolume.vertices[5] = Vector3(xMax + 1e-5, yMax + 1e-5, zMin - 1e-5);
		pMesh->m_boundingVolume.vertices[6] = Vector3(xMin - 1e-5, yMax + 1e-5, zMax + 1e-5);
		pMesh->m_boundingVolume.vertices[7] = Vector3(xMax + 1e-5, yMax + 1e-5, zMax + 1e-5);

		h = hMesh;
	}

	PEASSERT(h.isValid(), "Something must need to be loaded here");

	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
	return h;
}

void MeshManager::registerAsset(const PE::Handle &h)
{
	static int uniqueId = 0;
	++uniqueId;
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "__generated_%d", uniqueId);
	
	int index = m_assets.findIndex(key);
	PEASSERT(index == -1, "Generated meshes have to be unique");
	
	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
}

}; // namespace Components
}; // namespace PE
