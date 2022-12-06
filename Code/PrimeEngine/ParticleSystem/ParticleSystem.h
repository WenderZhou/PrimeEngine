#ifndef __PYENGINE_2_0_PARTICLE_SYSTEM_H__
#define __PYENGINE_2_0_PARTICLE_SYSTEM_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Scene/Mesh.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "../Events/Component.h"
#include "../Events/Event.h"
#include "../Events/StandardEvents.h"
#include "../Utils/Array/Array.h"
#include "../Scene/SceneNode.h"

// Sibling/Children includes
namespace PE {
namespace Components {

struct ParticleProperty
{
	Vector3 position;
	Vector3 positionVariation;

	Vector3 spawnerVelocity;

	Vector3 velocity;
	Vector3 velocityVariation;

	float lifeTime;
	float lifeTimeVariation;

	Vector4 colorBegin;
	Vector4 colorEnd;

	float sizeBegin;
	float sizeEnd;
};

struct Particle
{
	Vector3 position;
	Vector3 velocity;

	float lifeLeft;
	float lifeTime;

	Vector4 color;
	float size;

	bool valid;
};

struct ParticleMesh : public Mesh
{
	PE_DECLARE_CLASS(ParticleMesh);

	// Constructor -------------------------------------------------------------
	ParticleMesh(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself) : Mesh(context, arena, hMyself)
	{
		m_loaded = false;
	}

	virtual ~ParticleMesh(){}

	virtual void addDefaultComponents();

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_GATHER_DRAWCALLS);
	virtual void do_GATHER_DRAWCALLS(Events::Event *pEvt);

	void loadFrom3DPoints_needsRC(float *vals, int numParticles, const char *techName, int &threadOwnershipMask);

	PrimitiveTypes::Bool m_loaded;
	Handle m_meshCPU;
};

struct ParticleRenderer : public SceneNode
{
	PE_DECLARE_CLASS(ParticleRenderer);
	// Singleton ------------------------------------------------------------------

	static void Construct(PE::GameContext& context, PE::MemoryArena arena);

	static ParticleRenderer* Instance()
	{
		return s_myHandle.getObject<ParticleRenderer>();
	}

	static Handle InstanceHandle()
	{
		return s_myHandle;
	}

	static void SetInstanceHandle(const Handle& handle)
	{
		ParticleRenderer::s_myHandle = handle;
	}

	// Constructor -------------------------------------------------------------
	ParticleRenderer(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);
	virtual ~ParticleRenderer() {}

	// Methods      ------------------------------------------------------------
	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
	virtual void do_UPDATE(Events::Event* pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_GATHER_DRAWCALLS)
	virtual void do_PRE_GATHER_DRAWCALLS(Events::Event* pEvt);

	void emit();

	void postPreDraw(int& threadOwnershipMask);

	// Component ------------------------------------------------------------
	virtual void addDefaultComponents();

private:
	static Handle s_myHandle;
	int m_numFreeing;

	Handle m_hParticleMeshes[2];
	Handle m_hParticleMeshInstances[2];
	int m_currentlyDrawnParticleMesh;

	ParticleProperty m_particleProperty;

	#define MAX_PARTICLE_SIZE 100
	Particle m_particles[MAX_PARTICLE_SIZE];

	float emitCoolDown;
};

}; // namespace Components
}; // namespace PE
#endif
