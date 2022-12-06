#include "ParticleSystem.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"

#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/MeshManager.h"

namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(ParticleMesh, Mesh);

void ParticleMesh::addDefaultComponents()
{
	//add this handler before Mesh's handlers so we can intercept draw and modify transform
	PE_REGISTER_EVENT_HANDLER(Events::Event_GATHER_DRAWCALLS, do_GATHER_DRAWCALLS);
	Mesh::addDefaultComponents();
}

void ParticleMesh::loadFrom3DPoints_needsRC(float* vals, int numParticles, const char* techName, int& threadOwnershipMask)
{
	if (!m_meshCPU.isValid())
	{
		m_meshCPU = Handle("MeshCPU ParticleMesh", sizeof(MeshCPU));
		new (m_meshCPU) MeshCPU(*m_pContext, m_arena);
	}
	MeshCPU& mcpu = *m_meshCPU.getObject<MeshCPU>();

	if (!m_loaded)
	{
		mcpu.m_hPositionBufferCPU = Handle("VERTEX_BUFFER_CPU", sizeof(PositionBufferCPU));
		PositionBufferCPU* pvb = new(mcpu.m_hPositionBufferCPU) PositionBufferCPU(*m_pContext, m_arena);

		mcpu.m_hIndexBufferCPU = Handle("INDEX_BUFFER_CPU", sizeof(IndexBufferCPU));
		IndexBufferCPU* pib = new(mcpu.m_hIndexBufferCPU) IndexBufferCPU(*m_pContext, m_arena);
		pib->createBillboardCPUBuffer();

		mcpu.m_hTexCoordBufferCPU = Handle("TEXCOORD_BUFFER_CPU", sizeof(TexCoordBufferCPU));
		TexCoordBufferCPU* ptcb = new(mcpu.m_hTexCoordBufferCPU) TexCoordBufferCPU(*m_pContext, m_arena);

		mcpu.m_hColorBufferCPU = Handle("COLOR_BUFFER_CPU", sizeof(ColorBufferCPU));
		ColorBufferCPU* pcb = new(mcpu.m_hColorBufferCPU) ColorBufferCPU(*m_pContext, m_arena);

		mcpu.m_hMaterialSetCPU = Handle("MATERIAL_SET_CPU", sizeof(MaterialSetCPU));
		MaterialSetCPU* pmscpu = new(mcpu.m_hMaterialSetCPU) MaterialSetCPU(*m_pContext, m_arena);
		pmscpu->createSetWithOneTexturedMaterial("fire.dds", "Default", SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp);
	}
		
	//mcpu.createEmptyMesh();

	// this will cause not using the vertex buffer manager
	//so that engine always creates a new vertex buffer gpu and doesn't try to find and
	//existing one
	mcpu.m_manualBufferManagement = true;

	PositionBufferCPU* pVB = mcpu.m_hPositionBufferCPU.getObject<PositionBufferCPU>();
	TexCoordBufferCPU* pTCB = mcpu.m_hTexCoordBufferCPU.getObject<TexCoordBufferCPU>();
	ColorBufferCPU* pCB = mcpu.m_hColorBufferCPU.getObject<ColorBufferCPU>();
	IndexBufferCPU* pIB = mcpu.m_hIndexBufferCPU.getObject<IndexBufferCPU>();

	pVB->m_values.reset(numParticles * 4 * 3);
	pTCB->m_values.reset(numParticles * 4 * 2);
	pCB->m_values.reset(numParticles * 4 * 4);
	pIB->m_values.reset(numParticles * 6);

	pIB->m_indexRanges[0].m_start = 0;
	pIB->m_indexRanges[0].m_end = numParticles * 6 - 1;
	pIB->m_indexRanges[0].m_minVertIndex = 0;
	pIB->m_indexRanges[0].m_maxVertIndex = numParticles * 4 - 1;

	pIB->m_minVertexIndex = pIB->m_indexRanges[0].m_minVertIndex;
	pIB->m_maxVertexIndex = pIB->m_indexRanges[0].m_maxVertIndex;

	for (int ip = 0; ip < numParticles; ip++)
	{
		pVB->m_values.add(vals[ip * 28 + 0], vals[ip * 28 + 1], vals[ip * 28 + 2]);
		pVB->m_values.add(vals[ip * 28 + 3], vals[ip * 28 + 4], vals[ip * 28 + 5]);
		pVB->m_values.add(vals[ip * 28 + 6], vals[ip * 28 + 7], vals[ip * 28 + 8]);
		pVB->m_values.add(vals[ip * 28 + 9], vals[ip * 28 + 10], vals[ip * 28 + 11]);

		pTCB->m_values.add(0.5f, 0.125f);
		pTCB->m_values.add(0.625f, 0.125f);
		pTCB->m_values.add(0.625f, 0.25f);
		pTCB->m_values.add(0.5f, 0.25f);

		pCB->m_values.add(vals[ip * 28 + 12], vals[ip * 28 + 13], vals[ip * 28 + 14], vals[ip * 28 + 15]);
		pCB->m_values.add(vals[ip * 28 + 16], vals[ip * 28 + 17], vals[ip * 28 + 18], vals[ip * 28 + 19]);
		pCB->m_values.add(vals[ip * 28 + 20], vals[ip * 28 + 21], vals[ip * 28 + 22], vals[ip * 28 + 23]);
		pCB->m_values.add(vals[ip * 28 + 24], vals[ip * 28 + 25], vals[ip * 28 + 26], vals[ip * 28 + 27]);

		pIB->m_values.add(ip * 4 + 0, ip * 4 + 1, ip * 4 + 2);
		pIB->m_values.add(ip * 4 + 2, ip * 4 + 3, ip * 4 + 0);
	}

	if (!m_loaded)
	{
		// first time creating gpu mesh
		loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

		Handle hEffect = EffectManager::Instance()->getEffectHandle("ParticleMesh_Tech"); 
		
		for (int imat = 0; imat < m_effects.m_size; imat++)
		{
			if (m_effects[imat].m_size)
				m_effects[imat][0] = hEffect;
		}

		m_loaded = true;
	}
	else
	{
		//just need to update vertex buffers gpu
		updateGeoFromMeshCPU_needsRC(mcpu, threadOwnershipMask);
	}
}

void ParticleMesh::do_GATHER_DRAWCALLS(Events::Event* pEvt)
{

}

PE_IMPLEMENT_CLASS1(ParticleRenderer, SceneNode);

Handle ParticleRenderer::s_myHandle;

void ParticleRenderer::Construct(PE::GameContext& context, PE::MemoryArena arena)
{
	Handle handle("ParticleRenderer", sizeof(ParticleRenderer));
	ParticleRenderer* pParticleRenderer = new(handle) ParticleRenderer(context, arena, handle);
	pParticleRenderer->addDefaultComponents();
	// Singleton
	SetInstanceHandle(handle);
	RootSceneNode::Instance()->addComponent(handle);
}

ParticleRenderer::ParticleRenderer(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself)
: SceneNode(context, arena, hMyself)
{
	for (int i = 0; i < MAX_PARTICLE_SIZE; i++)
		m_particles[i].valid = false;

	emitCoolDown = 0.05f;

	m_particleProperty.position = Vector3(0.0f, 0.0f, 0.0f);
	m_particleProperty.positionVariation = Vector3(1.0f, 0.0f, 1.0f);

	m_particleProperty.spawnerVelocity = Vector3(0.5f, 0.0f, 0.0f);

	m_particleProperty.velocity = Vector3(0.0f, 1.0f, 0.0f);
	m_particleProperty.velocityVariation = Vector3(0.5f, 0.0f, 0.5f);

	m_particleProperty.lifeTime = 3.0f;
	m_particleProperty.lifeTimeVariation = 1.0f;

	m_particleProperty.colorBegin = Vector4(1.0f, 1.0f, 0.0f, 1.0f);
	m_particleProperty.colorEnd = Vector4(1.0f, 0.0f, 0.0f, 0.3f);

	m_particleProperty.sizeBegin = 1.0f;
	m_particleProperty.sizeEnd = 0.2f;
}

void ParticleRenderer::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, do_UPDATE);
	PE_REGISTER_EVENT_HANDLER(Events::Event_PRE_GATHER_DRAWCALLS, do_PRE_GATHER_DRAWCALLS);

	m_currentlyDrawnParticleMesh = 0;
	for (int i = 0; i < 2; i++)
	{
		// particle mesh
		m_hParticleMeshes[i] = PE::Handle("ParticleMesh", sizeof(ParticleMesh));
		ParticleMesh* pParticleMesh = new(m_hParticleMeshes[i]) ParticleMesh(*m_pContext, m_arena, m_hParticleMeshes[i]);
		pParticleMesh->addDefaultComponents();

		m_pContext->getMeshManager()->registerAsset(m_hParticleMeshes[i]);

		m_hParticleMeshInstances[i] = PE::Handle("ParticleMeshInstance", sizeof(MeshInstance));
		MeshInstance* pParticleMeshInstance = new(m_hParticleMeshInstances[i]) MeshInstance(*m_pContext, m_arena, m_hParticleMeshInstances[i]);
		pParticleMeshInstance->addDefaultComponents();

		pParticleMeshInstance->initFromRegisteredAsset(m_hParticleMeshes[i]);

		addComponent(m_hParticleMeshInstances[i]);

		pParticleMesh->setEnabled(false);

		pParticleMeshInstance->setEnabled(false);
	}
}

float inline random()
{
	return rand() * 1.0f / RAND_MAX - 0.5f;
}

void ParticleRenderer::do_UPDATE(Events::Event* pEvt)
{
	Events::Event_UPDATE* updateEvent = (Events::Event_UPDATE*)pEvt;

	m_particleProperty.position += m_particleProperty.spawnerVelocity * updateEvent->m_frameTime;

	if (m_particleProperty.position.m_x > 5.0f)
		m_particleProperty.spawnerVelocity.m_x = -0.5f;
	else if (m_particleProperty.position.m_x < -5.0f)
		m_particleProperty.spawnerVelocity.m_x = 0.5f;

	for (int i = 0; i < MAX_PARTICLE_SIZE; i++)
	{
		Particle& particle = m_particles[i];

		if (!particle.valid)
			continue;

		particle.lifeLeft -= updateEvent->m_frameTime;

		if (particle.lifeLeft < 0.0f)
		{
			particle.valid = false;
			continue;
		}

		particle.position += particle.velocity * updateEvent->m_frameTime;

		float t = particle.lifeLeft / particle.lifeTime;
		particle.color = m_particleProperty.colorBegin * t + m_particleProperty.colorEnd * (1 - t);
		particle.size = m_particleProperty.sizeBegin * t + m_particleProperty.sizeEnd * (1 - t);
	}

	if (emitCoolDown < 0)
	{
		for (int i = 0; i < MAX_PARTICLE_SIZE; i++)
		{
			Particle& particle = m_particles[i];

			if (particle.valid)
				continue;

			particle.valid = true;
			particle.position = Vector3(m_particleProperty.position.m_x + random() * m_particleProperty.positionVariation.m_x,
										m_particleProperty.position.m_y + random() * m_particleProperty.positionVariation.m_y,
										m_particleProperty.position.m_z + random() * m_particleProperty.positionVariation.m_z);
			particle.velocity = Vector3(m_particleProperty.velocity.m_x + random() * m_particleProperty.velocityVariation.m_x,
										m_particleProperty.velocity.m_y + random() * m_particleProperty.velocityVariation.m_y,
										m_particleProperty.velocity.m_z + random() * m_particleProperty.velocityVariation.m_z);
			particle.lifeTime = m_particleProperty.lifeTime + random() * m_particleProperty.lifeTimeVariation;
			particle.lifeLeft = particle.lifeTime;

			particle.color = m_particleProperty.colorBegin;
			particle.size = m_particleProperty.sizeBegin;

			emitCoolDown = 0.05f;
			break;
		}
	}
	else
	{
		emitCoolDown -= updateEvent->m_frameTime;
	}
}

void ParticleRenderer::do_PRE_GATHER_DRAWCALLS(Events::Event* pEvt)
{

}

void ParticleRenderer::postPreDraw(int& threadOwnershipMask)
{
	CameraSceneNode* pcam = CameraManager::Instance()->getActiveCamera()->getCamSceneNode();

	Array<float> vertexData(*m_pContext, m_arena);
	
	int validParticleCnt = 0;
	for (int i = 0; i < MAX_PARTICLE_SIZE; i++)
	{
		if (m_particles[i].valid)
			validParticleCnt++;
	}

	vertexData.reset(validParticleCnt * 28);

	Vector3 camU = pcam->m_worldTransform.getU();
	Vector3 camV = pcam->m_worldTransform.getV();

	for (int i = 0; i < MAX_PARTICLE_SIZE; i++)
	{
		Particle& particle = m_particles[i];

		if (particle.valid)
		{
			Vector3 vtx1 = particle.position - particle.size * camU + particle.size * camV;
			Vector3 vtx2 = particle.position + particle.size * camU + particle.size * camV;
			Vector3 vtx3 = particle.position + particle.size * camU - particle.size * camV;
			Vector3 vtx4 = particle.position - particle.size * camU - particle.size * camV;

			vertexData.add(vtx1.m_x); vertexData.add(vtx1.m_y); vertexData.add(vtx1.m_z);
			vertexData.add(vtx2.m_x); vertexData.add(vtx2.m_y); vertexData.add(vtx2.m_z);
			vertexData.add(vtx3.m_x); vertexData.add(vtx3.m_y); vertexData.add(vtx3.m_z);
			vertexData.add(vtx4.m_x); vertexData.add(vtx4.m_y); vertexData.add(vtx4.m_z);

			vertexData.add(particle.color.m_r); vertexData.add(particle.color.m_g); vertexData.add(particle.color.m_b); vertexData.add(particle.color.m_a);
			vertexData.add(particle.color.m_r); vertexData.add(particle.color.m_g); vertexData.add(particle.color.m_b); vertexData.add(particle.color.m_a);
			vertexData.add(particle.color.m_r); vertexData.add(particle.color.m_g); vertexData.add(particle.color.m_b); vertexData.add(particle.color.m_a);
			vertexData.add(particle.color.m_r); vertexData.add(particle.color.m_g); vertexData.add(particle.color.m_b); vertexData.add(particle.color.m_a);
		}
	}

	ParticleMesh* pParticleMesh = m_hParticleMeshes[m_currentlyDrawnParticleMesh].getObject<ParticleMesh>();
	MeshInstance* pParticleMeshInstance = m_hParticleMeshInstances[m_currentlyDrawnParticleMesh].getObject<MeshInstance>();
	// this mesh ahs been submitted to render already. we can disable it here (will nto be submitted on next frame)
	pParticleMesh->setEnabled(false);
	pParticleMeshInstance->setEnabled(false);

	m_currentlyDrawnParticleMesh = (m_currentlyDrawnParticleMesh + 1) % 2;
	pParticleMesh = m_hParticleMeshes[m_currentlyDrawnParticleMesh].getObject<ParticleMesh>();
	pParticleMeshInstance = m_hParticleMeshInstances[m_currentlyDrawnParticleMesh].getObject<MeshInstance>();

	if (vertexData.m_size)
	{
		pParticleMesh->loadFrom3DPoints_needsRC(vertexData.getFirstPtr(), validParticleCnt, "", threadOwnershipMask);
		pParticleMesh->setEnabled(true);
		pParticleMeshInstance->setEnabled(true);
	}
	else
	{
		pParticleMesh->setEnabled(false);
		pParticleMeshInstance->setEnabled(false);
	}
	vertexData.reset(0);
}


void ParticleRenderer::emit()
{
	
}

}; // namespace Components
}; // namespace PE