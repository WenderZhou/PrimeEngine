#ifndef HLSL_PARTICLEMESH_STRUCTS
#define HLSL_PARTICLEMESH_STRUCTS

#include "APIAbstraction.gpu"

// Vertex Shader  Input Structure /////////////////////////////////////////////////////////////////////////

struct PARTICLE_MESH_VS_IN
{
    float3 iPosL            API_SEMANTIC(VSIN_POSITION);
    float2 iTexCoord        API_SEMANTIC(TEXCOORD);
    float4 iColor          API_SEMANTIC(VSIN_COLOR);
};

struct PARTICLE_MESH_PS_IN
{
    float4 iPosH            API_SEMANTIC(PIPELINE_POSITION); //cggl: position
    float3 iPosW            API_SEMANTIC(PSIN_EXTRA_POSITION); // cggl: tc3
    float2 iTexCoord        API_SEMANTIC(TEXCOORD); // cggl tc0
    float4 iColor           API_SEMANTIC(PSIN_COLOR);
};

// Vertex Shader Function Definition //////////////////////////////////////////////////////////////////////

#if APIABSTRACTION_IOS

    //vertex inputs for IOS. will compile only for vertex shaders
    #if APIABSTRACTION_IOS_VERTEX
        attribute vec3 vIn_iPosL;
        attribute vec2 vIn_iTexCoord;
        attribute vec3 vIn_iNormal;
		
    #endif
#else
	#define VS_wrapper_PARTICLE_MESH(func) \
        PARTICLE_MESH_PS_IN main(PARTICLE_MESH_VS_IN API_NOSTRIP vIn) { \
            PARTICLE_MESH_PS_IN pIn; \
            pIn = func(vIn); \
            return pIn; \
        }
#endif


// Pixel Shader Function Defintion ////////////////////////////////////////////////////////////////////////////
#if APIABSTRACTION_IOS
    
    //vertex output = fragment input - will compile for both vertex and fragment shaders
    //varying float4 pIn_iPosH; use gl_Position
    varying float3 pIn_iPosW;
    varying float2 pIn_iTexCoord;
    varying float3 pIn_iNormalW;
	varying float4 pIn_iProjTexCoord;

#else
    #define PS_wrapper_PARTICLE_MESH(func) \
        float4 main(PARTICLE_MESH_PS_IN pIn) : PSOUT_COLOR \
        { \
            return func(pIn); \
        }
#endif

#endif // file guard
