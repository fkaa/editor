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

class ParticleSystem {
public:
	ParticleSystem(const wchar_t *file, UINT capacity, UINT width, UINT height, ID3D11Device *device, ID3D11DeviceContext *cxt);
	~ParticleSystem();

	void ProcessFX(ParticleEffect *fx, XMMATRIX model, float dt);
	void ProcessFX(ParticleEffect &fx, XMMATRIX model, XMVECTOR velocity, float dt);
	void AddFX(std::string name, XMMATRIX model);
	ParticleEffect GetFX(std::string name);

	void update(Camera *cam, float dt);
	void render(Camera *cam, CommonStates *states, ID3D11DepthStencilView *dst_dsv, ID3D11RenderTargetView *dst_rtv);
	void frame();

	void ReadSphereModel();

//private:
	UINT capacity;

	std::vector<ParticleEffect> m_ParticleEffectDefinitions;

	std::vector<ParticleEffectInstance> m_ParticleEffects;
	std::vector<BillboardParticle> m_BillboardParticles;
	std::vector<GeometryParticle> m_GeometryParticles;
	std::vector<Trail> m_TrailParticles;

	int m_GeometryIndices;
	VertexBuffer<SphereVertex> *m_GeometryBuffer;
	IndexBuffer<UINT16> *m_GeometryIndexBuffer;
	VertexBuffer<GeometryParticle> *m_GeometryInstanceBuffer;
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
