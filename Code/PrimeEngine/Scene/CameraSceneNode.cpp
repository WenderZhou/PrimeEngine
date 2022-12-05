#include "CameraSceneNode.h"
#include "../Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"

#define Z_ONLY_CAM_BIAS 0.0f
namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(CameraSceneNode, SceneNode);

CameraSceneNode::CameraSceneNode(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself) : SceneNode(context, arena, hMyself)
{
	m_near = 0.05f;
	m_far = 2000.0f;
}
void CameraSceneNode::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(Events::Event_CALCULATE_TRANSFORMATIONS, CameraSceneNode::do_CALCULATE_TRANSFORMATIONS);
}

void CameraSceneNode::do_CALCULATE_TRANSFORMATIONS(Events::Event *pEvt)
{
	Handle hParentSN = getFirstParentByType<SceneNode>();
	if (hParentSN.isValid())
	{
		Matrix4x4 parentTransform = hParentSN.getObject<PE::Components::SceneNode>()->m_worldTransform;
		m_worldTransform = parentTransform * m_base;
	}
	
	Matrix4x4 &mref_worldTransform = m_worldTransform;

	Vector3 pos = Vector3(mref_worldTransform.m[0][3], mref_worldTransform.m[1][3], mref_worldTransform.m[2][3]);
	Vector3 n = Vector3(mref_worldTransform.m[0][2], mref_worldTransform.m[1][2], mref_worldTransform.m[2][2]);
	Vector3 target = pos + n;
	Vector3 up = Vector3(mref_worldTransform.m[0][1], mref_worldTransform.m[1][1], mref_worldTransform.m[2][1]);

	m_worldToViewTransform = CameraOps::CreateViewMatrix(pos, target, up);

	m_worldTransform2 = mref_worldTransform;

	m_worldTransform2.moveForward(Z_ONLY_CAM_BIAS);

	Vector3 pos2 = Vector3(m_worldTransform2.m[0][3], m_worldTransform2.m[1][3], m_worldTransform2.m[2][3]);
	Vector3 n2 = Vector3(m_worldTransform2.m[0][2], m_worldTransform2.m[1][2], m_worldTransform2.m[2][2]);
	Vector3 target2 = pos2 + n2;
	Vector3 up2 = Vector3(m_worldTransform2.m[0][1], m_worldTransform2.m[1][1], m_worldTransform2.m[2][1]);

	m_worldToViewTransform2 = CameraOps::CreateViewMatrix(pos2, target2, up2);
    
    PrimitiveTypes::Float32 aspect = (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getWidth()) / (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getHeight());
    
    PrimitiveTypes::Float32 verticalFov = 0.33f * PrimitiveTypes::Constants::c_Pi_F32;
    if (aspect < 1.0f)
    {
        //ios portrait view
        static PrimitiveTypes::Float32 factor = 0.5f;
        verticalFov *= factor;
    }

	m_viewToProjectedTransform = CameraOps::CreateProjectionMatrix(verticalFov, 
		aspect,
		m_near, m_far);
	
	createFrustum(0.75 * verticalFov, aspect, pos, target, up);

	SceneNode::do_CALCULATE_TRANSFORMATIONS(pEvt);

}

void CameraSceneNode::createFrustum(float verticalFov, float aspect, Vector3 pos, Vector3 target, Vector3 up)
{
	Vector3 z = target - pos;
	Vector3 y = up;
	Vector3 x = y.crossProduct(z);
	y = z.crossProduct(x);

	x.normalize();
	y.normalize();
	z.normalize();

	float hh = tanf(verticalFov / 2);
	float hw = aspect * hh;

	Vector3 tr_view = Vector3(hw, hh, 1);
	Vector3 tl_view = Vector3(-hw, hh, 1);
	Vector3 br_view = Vector3(hw, -hh, 1);
	Vector3 bl_view = Vector3(-hw, -hh, 1);

	Vector3 tr_world = tr_view.m_x * x + tr_view.m_y * y + tr_view.m_z * z;
	Vector3 tl_world = tl_view.m_x * x + tl_view.m_y * y + tl_view.m_z * z;
	Vector3 br_world = br_view.m_x * x + br_view.m_y * y + br_view.m_z * z;
	Vector3 bl_world = bl_view.m_x * x + bl_view.m_y * y + bl_view.m_z * z;
	
	m_frustum.top.n = -tl_world.crossProduct(tr_world);
	m_frustum.bottom.n = -br_world.crossProduct(bl_world);
	m_frustum.left.n = -bl_world.crossProduct(tl_world);
	m_frustum.right.n = -tr_world.crossProduct(br_world);
	m_frustum.front.n = z;
	m_frustum.back.n = -z;

	m_frustum.top.n.normalize();
	m_frustum.bottom.n.normalize();
	m_frustum.left.n.normalize();
	m_frustum.right.n.normalize();
	m_frustum.front.n.normalize();
	m_frustum.back.n.normalize();

	m_frustum.top.d = -m_frustum.top.n * pos;
	m_frustum.bottom.d = -m_frustum.bottom.n * pos;
	m_frustum.left.d = -m_frustum.left.n * pos;
	m_frustum.right.d = -m_frustum.right.n * pos;
	m_frustum.front.d = -m_frustum.front.n * (m_near * tr_world + pos);
	m_frustum.back.d = -m_frustum.back.n * (m_far * tr_world + pos);

	return;
}

}; // namespace Components
}; // namespace PE
