#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "Camera.h"
#include "Ease.h"
#include "Particle.h"
#include <DirectXMath.h>

struct ParticleEffectInstance {
	XMVECTOR position;
	ParticleEffect effect;
};

class ParticleSystem {
public:
	ParticleSystem(const wchar_t *file, UINT capacity, UINT width, UINT height, ID3D11Device *device, ID3D11DeviceContext *cxt);
	~ParticleSystem();

	void ProcessFX(ParticleEffect &fx, XMMATRIX model, float dt);
	void ProcessFX(ParticleEffect &fx, XMMATRIX model, XMVECTOR velocity, float dt);
	void AddFX(std::string name, XMMATRIX model);
	ParticleEffect GetFX(std::string name);

	void update(Camera *cam, float dt);
	void renderShadows(
		ID3D11Buffer *shadow_camera,
		ID3D11Buffer *camera,
		ID3D11DepthStencilView *shadow_map,
		ID3D11DepthStencilState *shadow_depth_state
	);
	void render(
		Camera *cam,
		ID3D11RenderTargetView *dst_rtv,
		ID3D11ShaderResourceView *dst_srv,
		ID3D11RenderTargetView *dst_bright,
		ID3D11RenderTargetView *output,
		ID3D11DepthStencilView *dsv,
		ID3D11DepthStencilState *depth_state,
		bool debug = false
	);

//private:
	
	//ParticleEffect *getEffect(std::string name);
	UINT capacity;

	std::vector<ParticleDefinition> particle_definitions;
	std::vector<ParticleEffect> effect_definitions;

	std::vector<ParticleEffectInstance> effects;
	std::vector<ParticleInstance> particles;

	ID3D11RasterizerState *DefaultRaster;

	ID3D11BlendState *no_blend;
	ID3D11Buffer *particle_buffer;
	ID3D11InputLayout *particle_layout;

	ID3D11GeometryShader *shadow_gs;
	ID3D11PixelShader *shadow_ps;

	ID3D11VertexShader *particle_vs;
	ID3D11GeometryShader *particle_gs;
	ID3D11PixelShader *particle_ps;
	ID3D11ShaderResourceView *particle_srv;
	ID3D11ShaderResourceView *particle_debug_srv;
	ID3D11SamplerState *particle_sampler;

	ID3D11BlendState *particle_blend;

	ID3D11Buffer *composite_vertex_buffer;
	ID3D11InputLayout *composite_layout;

	ID3D11VertexShader *composite_vs;
	ID3D11PixelShader *composite_ps;
	ID3D11SamplerState *composite_sampler;


	ID3D11RenderTargetView *distort_rtv;
	ID3D11ShaderResourceView *distort_srv;

	ID3D11Device *device;
	ID3D11DeviceContext *cxt;
};

extern ParticleSystem *FXSystem;
