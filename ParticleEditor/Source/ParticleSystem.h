#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "Camera.h"
#include "Ease.h"
#include "Particle.h"
#include <DirectXMath.h>

#include <External\Helpers.h>
#include <External\DirectXTK.h>

using namespace DirectX;

struct ParticleEffectInstance {
	XMVECTOR position;
	ParticleEffect effect;
};

struct SphereVertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

struct DirectionalLight
{
    XMFLOAT4 position;
    XMFLOAT3 color;
    XMFLOAT3 ambient;
    float _padding[2];
};

struct Light
{
    XMFLOAT3 position;
    float    range;
    XMFLOAT3 color;
    float    intensity;
};


struct LightBuffer {
    uint32_t LightCount;
    uint32_t padding[3];
    Light Lights[128];
};
static int a = sizeof(LightBuffer);

class ParticleSystem {
public:
	ParticleSystem(const wchar_t *file, UINT capacity, UINT width, UINT height, ID3D11Device *device, ID3D11DeviceContext *cxt);
	~ParticleSystem();

	void ProcessAnchoredFX(AnchoredParticleEffect *fx, SimpleMath::Matrix model, float dt);
	void ProcessFX(ParticleEffect *fx, SimpleMath::Matrix model, float dt);
	void ProcessFX(ParticleEffect &fx, XMMATRIX model, XMVECTOR velocity, float dt);
	void AddFX(std::string name, XMMATRIX model);
	ParticleEffect GetFX(std::string name);

	void update(Camera *cam, float dt);
	void render(Camera *cam, CommonStates *states, ID3D11DepthStencilView *dst_dsv, ID3D11RenderTargetView *dst_rtv, bool debug);
	void frame();
    GeometryParticleInstance *UpdateParticles(XMVECTOR anchor, std::vector<GeometryParticle> &particles, float dt, GeometryParticleInstance *output, GeometryParticleInstance *max);

	void ReadSphereModel();

//private:
	UINT capacity;

	std::vector<ParticleEffect> m_ParticleEffectDefinitions;

	std::vector<ParticleEffectInstance> m_ParticleEffects;
	std::vector<AnchoredParticleEffect*> m_AnchoredEffects;

	std::vector<BillboardParticle> m_BillboardParticles;
	std::vector<GeometryParticle> m_GeometryParticles;
	std::vector<Trail> m_TrailParticles;

    LightBuffer m_ParticleLights;

    ConstantBuffer<DirectionalLight> *m_DirectionalLight;
    ConstantBuffer<LightBuffer> *m_Lights;
	int m_GeometryIndices;
	VertexBuffer<SphereVertex> *m_GeometryBuffer;
	IndexBuffer<UINT16> *m_GeometryIndexBuffer;
	VertexBuffer<GeometryParticleInstance> *m_GeometryInstanceBuffer;
	VertexBuffer<BillboardParticle> *m_BillboardBuffer;
	VertexBuffer<TrailParticle> *m_TrailBuffer;

	ID3D11BlendState *m_ParticleBlend;
	ID3D11InputLayout *m_DefaultBillboardLayout;
	ID3D11VertexShader *m_DefaultBillboardVS;

	ID3D11InputLayout *m_DefaultGeometryLayout;
	ID3D11VertexShader *m_DefaultGeometryVS;

	ID3D11GeometryShader *m_DefaultBillboardGS;


	ID3D11VertexShader   *trail_vs;
	ID3D11GeometryShader *trail_gs;
	ID3D11PixelShader    *trail_ps;
	ID3D11InputLayout *trail_layout;


	
	ID3D11Device *device;
	ID3D11DeviceContext *cxt;
};

extern ParticleSystem *FXSystem;
