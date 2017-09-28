#include "Editor.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <vector>
#include <algorithm>

#include "External/DirectXTK.h"
#include "External/imgui.h"
#include "External/imgui_dock.h"

#include "External/Helpers.h"
#include "External/dxerr.h"

#include "Camera.h"
#include "Ease.h"

#include "Particle.h"

#include "ParticleSystem.h"

#include "Globals.h"

using namespace DirectX;

void GetMipLevelSize(int startw, int starth, int level, int &outw, int &outh)
{
	while (level-- && !(startw == 1 && starth == 1)) {
		if (startw != 1)
			startw /= 2;

		if (starth != 1)
			starth /= 2;
	}

	outw = startw;
	outh = starth;
}

static ID3D11RenderTargetView *RESET_RTV[16] = {};
static ID3D11ShaderResourceView *RESET_SRV[16] = {};

Camera *camera;
float time;
float ptime;
float shadowznear = 1.f;
float shadowzfar = 30.f;
bool debug = false;
XMFLOAT3 directionalLightPos = { -1, 4, -1 };

XMFLOAT3 particlePos = { 0,0,0 };

ParticleSystem *FX;

ParticleEffect *current_effect;

Editor::Settings default_settings;
Editor::Settings settings;

D3D11_VIEWPORT viewport;
bool viewport_render;
bool viewport_dirty = true;


ID3D11DepthStencilState *DepthStateReadWrite;
ID3D11DepthStencilState *DepthStateRead;
ID3D11DepthStencilState *DepthStateDisable;

ID3D11DepthStencilView *ShadowMap;
ID3D11RasterizerState *ShadowRaster;
ID3D11RasterizerState *DefaultRaster;
ID3D11RasterizerState *DebugRaster;
Camera::BufferVals shadow_camera;
ID3D11Buffer *shadow_wvp_buffer;
ID3D11SamplerState *shadowMapSampler;
ID3D11RenderTargetView *shadowMapRTV;
ID3D11ShaderResourceView *shadowMapSRV;
ID3D11VertexShader *shadowMapVS;
ID3D11PixelShader *shadowMapPS;
ID3D11Buffer *dLightBuffer;


// Plane
ID3D11Buffer *plane_vertex_buffer;
ID3D11InputLayout *plane_layout;

ID3D11VertexShader *plane_vs;
ID3D11PixelShader *plane_ps;
ID3D11ShaderResourceView *plane_srv;
ID3D11SamplerState *plane_sampler;

ID3D11RenderTargetView *default_rtv;
ID3D11ShaderResourceView *default_srv;

ID3D11Buffer *blur_fs_vertices;
ID3D11VertexShader *blur_fs_vs;
ID3D11InputLayout *blur_fs_layout;
ID3D11SamplerState *blur_fs_sampler;


ID3D11ShaderResourceView *mip_start;

ID3D11RenderTargetView *mip_rtv[10];
ID3D11ShaderResourceView *mip_srv[10];
ID3D11PixelShader *passthrough_ps;

ID3D11PixelShader *gaussian_x_ps;
ID3D11PixelShader *gaussian_y_ps;

ID3D11PixelShader *blur_composite;

ID3D11BlendState *no_blend;

ID3D11RenderTargetView *distort_rtv;
ID3D11ShaderResourceView *distort_srv;
ID3D11ShaderResourceView *distortSRV;

ID3D11RenderTargetView *hdr_rtv;
ID3D11ShaderResourceView *hdr_srv;


// TEMP:
ID3D11VertexShader   *trail_vs;
ID3D11GeometryShader *trail_gs;
ID3D11PixelShader    *trail_ps;
ID3D11Buffer *trail_buffer;
ID3D11InputLayout *trail_layout;


inline float RandomFloat(float lo, float hi)
{
	return ((hi - lo) * ((float)rand() / RAND_MAX)) + lo;
}

void ComboFunc(const char *label, ParticleEase *ease)
{
	ImGui::Combo(label, (int*)ease, EASE_STRINGS);
}

bool ComboFuncOptional(const char *label, ParticleEase *ease)
{
	ImGui::Combo(label, (int*)ease, EASE_STRINGS_OPTIONAL);

	return (*(int*)ease) != 3;
}

void SetViewport(float width = EWIDTH, float height = EHEIGHT)
{
	D3D11_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gDeviceContext->RSSetViewports(1, &vp);
}

void InitShadows()
{
	/*D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(dirLight);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	DXCALL(gDevice->CreateBuffer(&desc, nullptr, &dLightBuffer));

	ID3D11Texture2D* depth_tex = NULL;
	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width = EWIDTH;
	descDepth.Height = EHEIGHT;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	descDepth.Width = 2048;
	descDepth.Height = 2048;
	descDepth.SampleDesc.Count = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

	DXCALL(gDevice->CreateTexture2D(&descDepth, NULL, &depth_tex));

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = false;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	DXCALL(gDevice->CreateDepthStencilState(&dsDesc, &DepthStateReadWrite));
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DXCALL(gDevice->CreateDepthStencilState(&dsDesc, &DepthStateRead));
	dsDesc.DepthEnable = false;
	DXCALL(gDevice->CreateDepthStencilState(&dsDesc, &DepthStateDisable));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	ZeroMemory(&descSRV, sizeof(descSRV));
	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Texture2D.MostDetailedMip = 0;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DXCALL(gDevice->CreateDepthStencilView(depth_tex, &descDSV, &ShadowMap));
	DXCALL(gDevice->CreateShaderResourceView(depth_tex, &descSRV, &shadowMapSRV));
	depth_tex->Release();





	D3D11_RASTERIZER_DESC state;
	ZeroMemory(&state, sizeof(D3D11_RASTERIZER_DESC));
	state.FillMode = D3D11_FILL_SOLID;
	state.CullMode = D3D11_CULL_FRONT;
	state.FrontCounterClockwise = false;
	state.DepthBias = 0;
	state.DepthBiasClamp = 0;
	state.SlopeScaledDepthBias = 0;
	state.DepthClipEnable = true;
	state.ScissorEnable = false;
	state.MultisampleEnable = false;
	state.AntialiasedLineEnable = false;

	DXCALL(gDevice->CreateRasterizerState(&state, &ShadowRaster));

	ZeroMemory(&state, sizeof(D3D11_RASTERIZER_DESC));
	state.FillMode = D3D11_FILL_WIREFRAME;
	state.CullMode = D3D11_CULL_NONE;
	state.FrontCounterClockwise = false;
	state.DepthBias = 0;
	state.DepthBiasClamp = 0;
	state.SlopeScaledDepthBias = 0;
	state.DepthClipEnable = true;
	state.ScissorEnable = false;
	state.MultisampleEnable = false;
	state.AntialiasedLineEnable = true;

	DXCALL(gDevice->CreateRasterizerState(&state, &DebugRaster));

	gDeviceContext->RSGetState(&DefaultRaster);

	D3D11_SAMPLER_DESC sampdesc;
	ZeroMemory(&sampdesc, sizeof(sampdesc));
	sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampdesc.MaxAnisotropy = 8;
	//sampdesc.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
	DXCALL(gDevice->CreateSamplerState(&sampdesc, &shadowMapSampler));

	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(Camera::BufferVals);
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
	data.pSysMem = &shadow_camera;

	DXCALL(gDevice->CreateBuffer(&desc, &data, &shadow_wvp_buffer));

	ID3DBlob *blob = compile_shader(L"../Pushlock/Shadow.hlsl", "VS", "vs_5_0", gDevice);
	DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shadowMapVS));

	/*D3D11_INPUT_ELEMENT_DESC input_desc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	debug_entity_layout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob, gDevice);

	blob = compile_shader(L"../Pushlock/Shadow.hlsl", "PS", "ps_5_0", gDevice);
	DXCALL(gDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shadowMapPS));*/

}

void InitPlane()
{
	float vertices[] = {
		-25.f, -0.1f, -25.f,
		-25.f, -0.1f,  25.f,
		25.f, -0.1f,  25.f,

		-25.f, -0.1f, -25.f,
		25.f, -0.1f,  25.f,
		25.f, -0.1f, -25.f
	};

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = (UINT)(sizeof(vertices));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
	data.pSysMem = &vertices[0];

	DXCALL(gDevice->CreateBuffer(&desc, &data, &plane_vertex_buffer));

	ID3DBlob *blob = compile_shader(L"Resources/Plane.hlsl", "VS", "vs_5_0", gDevice);
	DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &plane_vs));

	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	plane_layout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob, gDevice);

	blob = compile_shader(L"Resources/Plane.hlsl", "PS", "ps_5_0", gDevice);
	DXCALL(gDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &plane_ps));

	ID3D11Resource *r = nullptr;
	DXCALL(CreateDDSTextureFromFile(gDevice, L"Resources/Plane.dds", &r, &plane_srv, 0, nullptr));

	D3D11_SAMPLER_DESC sampdesc;
	ZeroMemory(&sampdesc, sizeof(sampdesc));
	sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.Filter = D3D11_FILTER_ANISOTROPIC;
	DXCALL(gDevice->CreateSamplerState(&sampdesc, &plane_sampler));

	ID3D11Texture2D *tex;
	D3D11_TEXTURE2D_DESC rtv_desc;
	rtv_desc.Width = EWIDTH;
	rtv_desc.Height = EHEIGHT;
	rtv_desc.Usage = D3D11_USAGE_DEFAULT;
	rtv_desc.MipLevels = 1;
	rtv_desc.ArraySize = 1;
	rtv_desc.SampleDesc.Count = 1;
	rtv_desc.SampleDesc.Quality = 0;
	rtv_desc.Format = DXGI_FORMAT_R16G16B16A16_TYPELESS;
	rtv_desc.CPUAccessFlags = 0;
	rtv_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtv_desc.MiscFlags = 0;

	DXCALL(gDevice->CreateTexture2D(&rtv_desc, nullptr, &tex));

	D3D11_SHADER_RESOURCE_VIEW_DESC sdesc;
	ZeroMemory(&sdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	D3D11_RENDER_TARGET_VIEW_DESC rdesc;
	ZeroMemory(&sdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	sdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	sdesc.Texture2D.MipLevels = 1;
	sdesc.Texture2D.MostDetailedMip = 0;

	rdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rdesc.Texture2D.MipSlice = 0;

	DXCALL(gDevice->CreateShaderResourceView(tex, &sdesc, &default_srv));
	DXCALL(gDevice->CreateRenderTargetView(tex, &rdesc, &default_rtv));
}

struct TrailParticle {
	SimpleMath::Vector3 position;
	SimpleMath::Vector2 size;
};


#define TRAIL_COUNT 32
TrailParticle particles[TRAIL_COUNT];

void InitParticles()
{
	FX = new ParticleSystem(nullptr, 4096, EWIDTH, EHEIGHT, gDevice, gDeviceContext);



	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = (UINT)(sizeof(TrailParticle) * TRAIL_COUNT * 4);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	DXCALL(gDevice->CreateBuffer(&desc, nullptr, &trail_buffer));

	ID3DBlob *blob = compile_shader(L"Resources/TrailParticleSimple.hlsl", "VS", "vs_5_0", gDevice);
	DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_vs));

	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	trail_layout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob, gDevice);

	blob = compile_shader(L"Resources/TrailParticleSimple.hlsl", "GS", "gs_5_0", gDevice);
	DXCALL(gDevice->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_gs));

	blob = compile_shader(L"Resources/TrailParticleSimple.hlsl", "PS", "ps_5_0", gDevice);
	DXCALL(gDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_ps));

}

void InitComposite()
{
	D3D11_SAMPLER_DESC sadesc;
	ZeroMemory(&sadesc, sizeof(sadesc));
	sadesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sadesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sadesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sadesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sadesc.MaxLOD = 11.f;
	DXCALL(gDevice->CreateSamplerState(&sadesc, &blur_fs_sampler));

	float vertices[] = {
		-1,  1, 0, 0,
		1, -1, 1, 1,
		-1, -1, 0, 1,

		1, -1, 1, 1,
		-1,  1, 0, 0,
		1,  1, 1, 0
	};

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = (UINT)(sizeof(vertices));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
	data.pSysMem = &vertices[0];

	DXCALL(gDevice->CreateBuffer(&desc, &data, &blur_fs_vertices));

	ID3DBlob *blob = compile_shader(L"GaussianPass.hlsl", "VS", "vs_5_0", gDevice);
	DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &blur_fs_vs));

	D3D11_INPUT_ELEMENT_DESC iinput_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	blur_fs_layout = create_input_layout(iinput_desc, ARRAYSIZE(iinput_desc), blob, gDevice);
	blob = compile_shader(L"GaussianPass.hlsl", "PS", "ps_5_0", gDevice);
	DXCALL(gDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &passthrough_ps));

	blob = compile_shader(L"GlowComposite.hlsl", "PS", "ps_5_0", gDevice);
	DXCALL(gDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &blur_composite));
	
	ID3D11Texture2D *tex;
	D3D11_TEXTURE2D_DESC rtv_desc;
	ZeroMemory(&rtv_desc, sizeof(rtv_desc));
	rtv_desc.Width = EWIDTH;
	rtv_desc.Height = EHEIGHT;
	rtv_desc.Usage = D3D11_USAGE_DEFAULT;
	rtv_desc.MipLevels = 1;
	rtv_desc.ArraySize = 1;
	rtv_desc.SampleDesc.Count = 1;
	rtv_desc.SampleDesc.Quality = 0;
	rtv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtv_desc.CPUAccessFlags = 0;
	rtv_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtv_desc.MiscFlags = 0;

	DXCALL(gDevice->CreateTexture2D(&rtv_desc, nullptr, &tex));

	DXCALL(gDevice->CreateShaderResourceView(tex, nullptr, &hdr_srv));
	DXCALL(gDevice->CreateRenderTargetView(tex, nullptr, &hdr_rtv));

	ID3D11Texture2D *dtex;
	ZeroMemory(&rtv_desc, sizeof(rtv_desc));
	rtv_desc.Width = EWIDTH;
	rtv_desc.Height = EHEIGHT;
	rtv_desc.Usage = D3D11_USAGE_DEFAULT;
	rtv_desc.MipLevels = 11;
	rtv_desc.ArraySize = 1;
	rtv_desc.SampleDesc.Count = 1;
	rtv_desc.SampleDesc.Quality = 0;
	rtv_desc.Format = DXGI_FORMAT_R16G16B16A16_TYPELESS;
	rtv_desc.CPUAccessFlags = 0;
	rtv_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtv_desc.MiscFlags = 0; 

	DXCALL(gDevice->CreateTexture2D(&rtv_desc, nullptr, &dtex));

	D3D11_SHADER_RESOURCE_VIEW_DESC sdesc;
	ZeroMemory(&sdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	sdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	sdesc.Texture2D.MipLevels = 11;
	sdesc.Texture2D.MostDetailedMip = 0;

	D3D11_RENDER_TARGET_VIEW_DESC rdesc;
	ZeroMemory(&rdesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	rdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rdesc.Texture2D.MipSlice = 0;

	DXCALL(gDevice->CreateShaderResourceView(dtex, &sdesc, &distort_srv));
	DXCALL(gDevice->CreateRenderTargetView(dtex, &rdesc,   &distort_rtv));

	sdesc.Texture2D.MipLevels = 1;
	DXCALL(gDevice->CreateShaderResourceView(dtex, &sdesc, &mip_start));


	for (int i = 0; i < 10; ++i) {
		sdesc.Texture2D.MostDetailedMip = i + 1;
		sdesc.Texture2D.MipLevels = 1;
		rdesc.Texture2D.MipSlice = i + 1;

		DXCALL(gDevice->CreateShaderResourceView(dtex, &sdesc, &mip_srv[i]));
		DXCALL(gDevice->CreateRenderTargetView(dtex, &rdesc, &mip_rtv[i]));
	}

}

void RenderPlane()
{
	UINT32 stride = sizeof(float) * 3;
	UINT32 offset = 0u;

	gDeviceContext->IASetInputLayout(plane_layout);
	gDeviceContext->IASetVertexBuffers(0, 1, &plane_vertex_buffer, &stride, &offset);
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	gDeviceContext->VSSetShader(plane_vs, nullptr, 0);
	gDeviceContext->VSSetConstantBuffers(0, 1, &camera->wvp_buffer);

	gDeviceContext->PSSetShader(plane_ps, nullptr, 0);
	gDeviceContext->PSSetSamplers(0, 1, &plane_sampler);
	gDeviceContext->PSSetShaderResources(0, 1, &plane_srv);
	gDeviceContext->PSSetConstantBuffers(2, 1, &dLightBuffer);
	gDeviceContext->PSSetConstantBuffers(5, 1, &shadow_wvp_buffer);
	gDeviceContext->PSSetShaderResources(1, 1, &shadowMapSRV);
	gDeviceContext->PSSetSamplers(1, 1, &shadowMapSampler);

	gDeviceContext->OMSetRenderTargets(1, &default_rtv, gDepthbufferDSV);

	gDeviceContext->Draw(6, 0);

	ID3D11RenderTargetView *rtv = nullptr;
	gDeviceContext->OMSetRenderTargets(1, &rtv, nullptr);
}

void RenderMips()
{
	gDeviceContext->PSSetShaderResources(0, 1, RESET_SRV);
	gDeviceContext->OMSetRenderTargets(1, RESET_RTV, nullptr);

	UINT32 stride = sizeof(float) * 4;
	UINT32 offset = 0u;

	gDeviceContext->IASetInputLayout(blur_fs_layout);
	gDeviceContext->IASetVertexBuffers(0, 1, &blur_fs_vertices, &stride, &offset);
	gDeviceContext->VSSetShader(blur_fs_vs, nullptr, 0);

	gDeviceContext->PSSetShader(passthrough_ps, nullptr, 0);

	gDeviceContext->PSSetSamplers(0, 1, &blur_fs_sampler);
	gDeviceContext->PSSetShaderResources(0, 1, &mip_start);

	for (int i = -1; i < 9; ++i) {
		if (i >= 0)
			gDeviceContext->PSSetShaderResources(0, 1, &mip_srv[i]);
		gDeviceContext->OMSetRenderTargets(1, &mip_rtv[i + 1], nullptr);

		int mipw, miph;
		GetMipLevelSize(EWIDTH, EHEIGHT, i + 2, mipw, miph);
		SetViewport(mipw, miph);
		gDeviceContext->Draw(6, 0);

		gDeviceContext->PSSetShaderResources(0, 1, RESET_SRV);
		gDeviceContext->OMSetRenderTargets(1, RESET_RTV, nullptr);
	}

	SetViewport(EWIDTH, EHEIGHT);
}

void RenderParticles()
{
	if (debug)
		gDeviceContext->RSSetState(DebugRaster);
	else
		gDeviceContext->RSSetState(DefaultRaster);

	FX->render(reinterpret_cast<Camera*>(camera), default_rtv, default_srv, distort_rtv, hdr_rtv, gDepthbufferDSV, DepthStateRead, debug);
	if (debug)
		gDeviceContext->RSSetState(DebugRaster);
	else
		gDeviceContext->RSSetState(DefaultRaster);
	UINT stride = sizeof(TrailParticle);
	UINT offset = 0;// 1 * sizeof(TrailParticle);

	gDeviceContext->IASetVertexBuffers(0, 1, &trail_buffer, &stride, &offset);
	gDeviceContext->IASetInputLayout(trail_layout);
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
	gDeviceContext->VSSetShader(trail_vs, nullptr, 0);
	gDeviceContext->GSSetShader(trail_gs, nullptr, 0);
	gDeviceContext->PSSetShader(trail_ps, nullptr, 0);
	gDeviceContext->OMSetRenderTargets(1, &hdr_rtv, nullptr);
	gDeviceContext->Draw(TRAIL_COUNT, 0);
	
	gDeviceContext->GSSetShader(nullptr, nullptr, 0);
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	gDeviceContext->RSSetState(DefaultRaster);

	RenderMips();
}

void RenderComposite()
{
	UINT32 stride = sizeof(float) * 4;
	UINT32 offset = 0u;

	gDeviceContext->IASetInputLayout(blur_fs_layout);
	gDeviceContext->IASetVertexBuffers(0, 1, &blur_fs_vertices, &stride, &offset);
	gDeviceContext->VSSetShader(blur_fs_vs, nullptr, 0);

	gDeviceContext->PSSetSamplers(0, 1, &blur_fs_sampler);

	// todo: remove, gammalt
	/*gDeviceContext->OMSetRenderTargets(1, &blur_rtv[1], nullptr);
	gDeviceContext->PSSetShader(gaussian_x_ps, nullptr, 0);
	gDeviceContext->PSSetShaderResources(0, 1, &blur_srv[0]);
	gDeviceContext->Draw(6, 0);

	gDeviceContext->PSSetShaderResources(0, 1, RESET_SRV);
	gDeviceContext->OMSetRenderTargets(1, RESET_RTV, nullptr);

	gDeviceContext->OMSetRenderTargets(1, &blur_rtv[0], nullptr);
	gDeviceContext->PSSetShader(gaussian_y_ps, nullptr, 0);
	gDeviceContext->PSSetShaderResources(0, 1, &blur_srv[1]);
	gDeviceContext->Draw(6, 0);*/

	gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, nullptr);
	gDeviceContext->PSSetShader(blur_composite, nullptr, 0);
	gDeviceContext->PSSetShaderResources(0, 1, &hdr_srv);
	gDeviceContext->PSSetShaderResources(1, 1, &distort_srv);
	gDeviceContext->Draw(6, 0);
}

namespace Editor {

static bool menu_open = false;
static int current_fx = 0;
static int current_def = 0;

void Style()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.19f, 0.95f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.19f, 0.95f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.41f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.48f, 0.78f, 0.32f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.95f, 0.92f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.92f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.67f, 0.40f, 0.40f, 0.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.50f, 0.50f, 0.90f, 0.50f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	style.AntiAliasedLines = false;

	style.WindowRounding = 0.f;
	style.ScrollbarRounding = 0.f;
	style.ScrollbarSize = 15;
}

void MenuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("open")) {
			OPENFILENAME ofn;
			wchar_t szFileName[MAX_PATH] = L"";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = wndHandle;
			ofn.lpstrFilter = L"No Files (*.no)\0*.no\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = szFileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = L"no";

			if (GetOpenFileName(&ofn)) {

				FX->effect_definitions.clear();
				FX->particle_definitions.clear();

				DeserializeParticles(ofn.lpstrFile, FX->effect_definitions, FX->particle_definitions);
			}
		}
		if (ImGui::MenuItem("save", nullptr, nullptr, !FX->particle_definitions.empty() || !FX->effect_definitions.empty())) {
			OPENFILENAME ofn;
			wchar_t szFileName[MAX_PATH] = L"";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = wndHandle;
			ofn.lpstrFilter = L"No Files (*.no)\0*.no\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = szFileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
			ofn.lpstrDefExt = L"no";

			if (GetSaveFileName(&ofn)) {
				SerializeParticles(ofn.lpstrFile, FX->effect_definitions, FX->particle_definitions);
			}
		}
		if (ImGui::MenuItem("save editor layout")) {
			ImGui::SaveDock();
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Settings")) {
		ImGui::TextDisabled("Camera");
		ImGui::SliderFloat("distance", &settings.CameraDistance, 0.1f, 16.f);
		ImGui::SliderFloat("height", &settings.CameraHeight, 0.0f, 16.f);
		ImGui::SliderFloat("speed", &settings.CameraSpeed, -1.5f, 1.5f);
		ImGui::TextDisabled("Simulation");
		ImGui::SliderFloat("X", &particlePos.x, -5, 5);
		ImGui::SliderFloat("Y", &particlePos.y, 0, 10);
		ImGui::SliderFloat("Z", &particlePos.z, -5, 5);
		ImGui::SliderFloat("speed##part", &settings.ParticleSpeed, -1.5f, 1.5f);
		ImGui::DragFloat("time", &time, 0.01);
		ImGui::Checkbox("paused", &settings.ParticlePaused);
		ImGui::Checkbox("debug", &debug);

		if (ImGui::Button("reset")) {
			settings = default_settings;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void ParticleEditor()
{
	if (ImGui::BeginDock("Particles")) {
		ImGui::BeginGroup();
		ImGui::BeginChild("list", ImVec2(120, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < FX->particle_definitions.size(); i++)
		{
			char label[64];
			sprintf_s(label, 64, "%s##%d", FX->particle_definitions[i].name, i);
			if (ImGui::Selectable(label, current_def == i)) {
				current_def = i;
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
		if (!FX->particle_definitions.empty()) {
			ParticleDefinition *def = &FX->particle_definitions[current_def];

			ImGui::Text("%s", def->name);
			ImGui::Separator();
			ImGui::InputText("name", def->name, 32);
			ImGui::Combo("type", (int*)&def->orientation, "Planar\0Clip\0Velocity\0VelocityAnchored\0");


			ImGui::InputFloat("gravity", &def->gravity);
			ImGui::InputFloat("lifetime", &def->lifetime);

			ImGui::TextDisabled("Scale");
			ComboFunc("ease##scale", &def->scale_fn);
			ImGui::DragFloat("start##scale", &def->scale_start, 0.001f);
			ImGui::DragFloat("end##scale", &def->scale_end, 0.001f);

			bool glow = (int)def->distort_fn & (1 << 31);
			if (ImGui::Checkbox("Glow", &glow)) {
				int *fn = (int*)&def->distort_fn;
				*fn ^= 1 << 31;
			}
			glow = (int)def->distort_fn & (1 << 31);

			if (glow)
				ImGui::TextDisabled("Glow");
			else
				ImGui::TextDisabled("Distort");

			auto old = def->distort_fn;
			
			int *fn = (int*)&def->distort_fn;
			*fn &= ~(1 << 31);
			if (ComboFuncOptional("ease##diostort", &def->distort_fn)) {
				ImGui::DragFloat("start##dsdsdcolor", &def->distort_start, 0.1f, 0.f, 100.0f);
				ImGui::DragFloat("end##dsdsdcolor", &def->distort_end, 0.1, 0.f, 100.0f);
			}

			if (glow) {
				*fn |= (1 << 31);
			}

			ImGui::TextDisabled("Color");
			if (ComboFuncOptional("ease##color", &def->color_fn)) {
				ImGui::ColorEdit4("start##color", (float*)&def->start_color);
				ImGui::ColorEdit4("end##color", (float*)&def->end_color);
			}

			ImGui::TextDisabled("Texture");
			ImGui::InputInt4("uv", &def->u);

			float limit = ImGui::CalcItemWidth();

			auto size = (1 > (def->u2 / def->v2)) ?
				ImVec2(def->u2 * (limit / (float)def->v2), limit) :
				ImVec2(limit, def->v2 * (limit / (float)def->u2));

			ImGui::Image((void*)FX->particle_srv, size, ImVec2(def->u / 2048.f, def->v / 2048.f), ImVec2((def->u + def->u2) / 2048.f, (def->v + def->v2) / 2048.f), ImVec4(def->start_color.x, def->start_color.y, def->start_color.z, def->start_color.w));
		}
		ImGui::EndChild();

		ImGui::BeginChild("buttons");
		if (ImGui::Button("Add")) {
			FX->particle_definitions.push_back({});
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {

		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
	ImGui::EndDock();

}

void FXEditor()
{
	if (ImGui::BeginDock("FX")) {
		ImGui::BeginGroup();
		ImGui::BeginChild("list", ImVec2(120, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < FX->effect_definitions.size(); i++)
		{
			char label[64];
			sprintf_s(label, 64, "%s##%d", FX->effect_definitions[i].name, i);
			if (ImGui::Selectable(label, current_fx == i)) {

				current_fx = i;
				FX->effect_definitions[current_fx].age = 0.f;

			}
		}
		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
		if (!FX->effect_definitions.empty()) {
			ParticleEffect *fx = &FX->effect_definitions[current_fx];
			current_effect = fx;

			ImGui::Text("%s", fx->name);
			ImGui::Separator();
			ImGui::InputText("name##fx", fx->name, 32);

			ImGui::TextDisabled("Time");
			if (ImGui::Checkbox("loop", &fx->loop)) {
				fx->clamp_children = false;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("clamp to children", &fx->clamp_children)) {
				fx->loop = false;
			}

			if (fx->clamp_children) {
				for (int i = 0; i < current_effect->fx_count; ++i) {
					ParticleEffectEntry entry = fx->fx[i];
					auto idx = entry.idx & ~(1 << 31);

					if (idx < 0) continue;

					auto def = FX->particle_definitions[idx];

					fx->children_time = max(fx->children_time, entry.end);
				}
			}

			if (!fx->loop && !fx->clamp_children) {
				ImGui::DragFloat("duration", &fx->time, 0.025f, 0.f, 100000.f, "%.3f seconds");
			}

			ImGui::Separator();

			for (int i = 0; i < current_effect->fx_count; ++i) {
				ParticleEffectEntry *entry = &fx->fx[i];
				auto idx = entry->idx & ~(1 << 31);

				if (idx < 0) continue;

				auto def = FX->particle_definitions[idx];

				char label[64];
				sprintf_s(label, 64, "%s##%d", def.name, i);

				bool header = ImGui::TreeNode(label);
				ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - 50);
				ImGui::PushID(i);
				if (ImGui::SmallButton("Remove")) {
					for (int j = i; j < current_effect->fx_count - 1; j++) {
						fx->fx[i] = fx->fx[j + 1];
					}
					fx->fx[current_effect->fx_count - 1] = {};
					fx->fx_count--;
				}
				ImGui::PopID();

				if (header) {
					ImGui::TextDisabled("Time");
					ImGui::Checkbox("loop##psoda", &entry->loop);

					if (entry->loop) {
						ImGui::DragFloat("start", &entry->start, 0.01, 0.f, 10000.f, "%.3f sec");
					}
					else {
						ImGui::DragFloat("start", &entry->start, 0.01, 0.f, 10000.f, "%.3f sec");
						ImGui::DragFloat("duration", &entry->end, 0.01, 0.f, 10000.f, "%.3f sec");
					}

					bool shadow = (int)entry->idx & (1 << 31);
					if (ImGui::Checkbox("Shadow", &shadow)) {
						int *fn = (int*)&entry->idx;
						*fn ^= 1 << 31;
					}

					ImGui::TextDisabled("Emitter");
					ImGui::Combo("type##emitter", (int*)&entry->emitter_type, EMITTER_STRINGS);
					switch (entry->emitter_type) {
					case ParticleEmitter::Cube:
						ImGui::TextDisabled("Spawn area");
						ImGui::DragFloatRange2("x", &entry->emitter_xmin, &entry->emitter_xmax, 0.01f);
						ImGui::DragFloatRange2("y", &entry->emitter_ymin, &entry->emitter_ymax, 0.01f);
						ImGui::DragFloatRange2("z", &entry->emitter_zmin, &entry->emitter_zmax, 0.01f);

						ImGui::TextDisabled("Spawn velocity");
						ImGui::DragFloatRange2("x##spawn", &entry->vel_xmin, &entry->vel_xmax, 0.01f);
						ImGui::DragFloatRange2("y##spawn", &entry->vel_ymin, &entry->vel_ymax, 0.01f);
						ImGui::DragFloatRange2("z##spawn", &entry->vel_zmin, &entry->vel_zmax, 0.01f);
						break;
					case ParticleEmitter::Sphere:
						break;
					}

					ImGui::TextDisabled("Rotation");
					ImGui::DragFloatRange2("start##rotsp", &entry->rot_min, &entry->rot_max, 0.01f);
					ImGui::DragFloatRange2("velocity##rotps", &entry->rot_vmin, &entry->rot_vmax, 0.01f);

					if (entry->emitter_type != ParticleEmitter::Static) {
						ImGui::TextDisabled("Spawn rate");
						ComboFunc("ease##spawn", &entry->spawn_fn);

						ImGui::DragInt("start##sffffs", &entry->spawn_start, 1.0f, 0, 500, "%.0f particles/s");
						ImGui::DragInt("end##sdasd", &entry->spawn_end, 1.0f, 0, 500, "%.0f particles/s");
					}

					ImGui::TreePop();
				}
			}

			ImGui::Separator();

			static int add_pdef = 0;

			if (ImGui::Button("Add##entry")) {
				ImGui::OpenPopup("Add particle");
				add_pdef = 0;
			}

			if (ImGui::BeginPopupModal("Add particle")) {

				ImGui::BeginChild("list popiup", ImVec2(150, 200), true);
				for (int i = 0; i < FX->particle_definitions.size(); i++)
				{
					char label[64];
					sprintf_s(label, 64, "%s##asd%d", FX->particle_definitions[i].name, i);
					if (ImGui::Selectable(label, add_pdef == i)) {
						add_pdef = i;
					}
				}
				ImGui::EndChild();
				ImGui::SameLine();
				if (ImGui::Button("Add##pop")) {
					auto &entry = current_effect->fx[current_effect->fx_count++];
					entry.idx = add_pdef;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel##popup")) {
					ImGui::CloseCurrentPopup();
				}



				ImGui::EndPopup();
			}
		}

		ImGui::EndChild();

		ImGui::BeginChild("buttons");
		if (ImGui::Button("Add##fx")) {
			ParticleEffect fx = {};
			FX->effect_definitions.push_back(fx);
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {

		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
	ImGui::EndDock();

	if (!FX->effect_definitions.empty()) {
		ParticleEffect *fx = &FX->effect_definitions[current_fx];
		current_effect = fx;
	}
}

void Timeline()
{
	if (ImGui::BeginDock("Timeline")) {

		ImGui::BeginGroup();

		ImGui::BeginChild("playstuff", ImVec2(150, 0), true);

		ImGui::Checkbox("paused##controls", &settings.ParticlePaused);
		ImGui::SameLine();
		ImGui::Checkbox("loop##controls", &settings.ParticleLoop);

		ImGui::SliderFloat("speed##part", &settings.ParticleSpeed, -1.5f, 1.5f);



		ImGui::EndChild();

		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		if (!FX->effect_definitions.empty()) {
			auto fx = FX->effect_definitions[current_fx];
			auto time = fx.clamp_children ? fx.children_time : fx.time;

			ImGui::ProgressBar(fx.age / time);
			ImGui::NewLine();

			auto hi = (int)ceilf(time);

			for (int i = 0; i < (int)hi; ++i) {
				if (i <= (int)time) {

					ImGui::SameLine(ImGui::GetContentRegionAvailWidth() * (i / (float)time));
					ImGui::TextDisabled("%.1f", (float)i);
				}
			}

			ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - 20);
			ImGui::TextDisabled("%.1f", time);
		}
		else {
			ImGui::ProgressBar(0.f);
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();



		auto h = ImGui::GetContentRegionAvail().y;
		auto cur = ImGui::GetCursorScreenPos();

		ImGui::BeginChild("timelineview", ImVec2(0, 0), true);

		auto acur = ImGui::GetCursorScreenPos();
		auto w = ImGui::GetContentRegionAvailWidth();

		if (!FX->effect_definitions.empty()) {
			auto fx = FX->effect_definitions[current_fx];
			auto time = fx.clamp_children ? fx.children_time : fx.time;
			auto hi = (int)ceilf(time);

			for (int i = 0; i < (int)hi; ++i) {
				auto extend = 4.f;
				if (i <= (int)time) {
					draw_list->AddLine(
						ImVec2(acur.x + w * (i / (float)time), cur.y - extend),
						ImVec2(acur.x + w * (i / (float)time), cur.y + h + extend),
						ImColor(ImGui::GetStyle().Colors[ImGuiCol_Border]),
						2.f
					);
				}

				for (int j = 0; j < 4; ++j) {
					draw_list->AddLine(
						ImVec2(acur.x + w * (((float)i + j / 4.f) / (float)time), cur.y),
						ImVec2(acur.x + w * (((float)i + j / 4.f) / (float)time), cur.y + h),
						ImColor(ImGui::GetStyle().Colors[ImGuiCol_Border]),
						1.f
					);
				}
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 3));


			for (int i = 0; i < current_effect->fx_count; ++i) {
				ParticleEffectEntry entry = current_effect->fx[i];
				auto idx = entry.idx & ~(1 << 31);

				if (idx < 0) continue;

				auto def = FX->particle_definitions[idx];
				ImGui::SameLine(w * (entry.start / time));

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 1.0, 1.0, 0.23));
				if (entry.loop)
					ImGui::Button(def.name, ImVec2(w, 0));
				else
					ImGui::Button(def.name, ImVec2(w * (entry.end) / time, 0));
				ImGui::PopStyleColor(1);


				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 1.0, 1.0, 0.1));
				if (!entry.loop) {
					ImGui::SameLine();
					ImGui::Button("lifetime", ImVec2(w * (def.lifetime) / time, 0));
				}
				ImGui::PopStyleColor(1);

				ImGui::Separator();
				ImGui::NewLine();
			}

			ImGui::PopStyleVar(1);
		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
	ImGui::EndDock();
}

void Init()
{
	ImGui::LoadDock();

	camera = new Camera({0.f, 0.5f, 0.f}, {});

	Style();

	InitShadows();
	InitPlane();
	InitParticles();
	InitComposite();
}

float a = 0;
void Update(float dt)
{
	if (!settings.ParticlePaused) {
		time += dt * settings.CameraSpeed;
		ptime += dt * settings.ParticleSpeed;
	}

	MenuBar();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::RootDock(ImVec2(0, 18), ImVec2(EWIDTH, EHEIGHT - 18));
	ParticleEditor();
	FXEditor();
	Timeline();

	viewport_render = false;

	if (ImGui::BeginDock("Viewport")) {
		viewport_render = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));

		ImGui::BeginChild("vp render", ImVec2(0, 0), true);

		auto cur = ImGui::GetCursorScreenPos();
		auto sz = ImGui::GetContentRegionAvail();

		if (cur.x != viewport.TopLeftX ||
			cur.y != viewport.TopLeftY ||
			sz.x != viewport.Width ||
			sz.y != viewport.Height)
		{
			viewport_dirty = true;

			viewport.Width = sz.x;
			viewport.Height = sz.y;
			viewport.MaxDepth = 1.0f;
			viewport.MinDepth = 0.0f;
			viewport.TopLeftX = cur.x;
			viewport.TopLeftY = cur.y;
		}

		if (viewport_dirty && !ImGui::IsMouseDragging()) {
			viewport_dirty = false;

			gDeviceContext->RSSetViewports(1, &viewport);
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	ImGui::EndDock();
	ImGui::PopStyleColor();

	auto pdt = dt * settings.ParticleSpeed;

	auto fx = current_effect;

	if (fx && !settings.ParticlePaused)
		FX->ProcessFX(*fx, XMMatrixTranslation(particlePos.x, particlePos.y, particlePos.z), pdt);

	if (!settings.ParticlePaused && settings.ParticleLoop && fx != nullptr) {
		auto time = fx->clamp_children ? fx->children_time : fx->time;

		if (fx->age >= time) {
			for (int i = 0; i < fx->fx_count; ++i) {
				auto &entry = fx->fx[i];
				if (entry.emitter_type == ParticleEmitter::Static)
					entry.spawned_particles = 1.f;
			}
			fx->age = 0;
		}
	}

	camera->pos = { sin(time) * settings.CameraDistance, settings.CameraHeight, cos(time) * settings.CameraDistance };
	camera->update(dt, viewport.Width, viewport.Height);
	if (!settings.ParticlePaused)
		FX->update(reinterpret_cast<Camera*>(camera), pdt);
	a += dt;
	if (a >= 0.015) {
		a -=0.015;
		for (int i = TRAIL_COUNT - 2; i > 1; i--) {
			//particles[i - 1].position = SimpleMath::Vector3::Lerp(particles[i].position, particles[i - 1].position, 0.9f);
			particles[i] = particles[i - 1];
		}
		particles[1] = {
			SimpleMath::Vector3(sin(-time *6)*2, 0.5, cos(-time *6)*2),
			SimpleMath::Vector2(0.3, 1.0)
		};
		particles[0] = particles[1];
		particles[TRAIL_COUNT - 1] = particles[TRAIL_COUNT - 2];
	}

	for (int i = 1; i < TRAIL_COUNT - 1; i++) {
		particles[i].position += SimpleMath::Vector3(0, 1 * dt, 0);
		particles[i].size = SimpleMath::Vector2(0.1, 1.0);
	}

	D3D11_MAPPED_SUBRESOURCE data = {};
	gDeviceContext->Map(trail_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
	{
		TrailParticle *particle = (TrailParticle *)data.pData;
		for (int i = 1; i < TRAIL_COUNT - 1; i++) {
			particle[i] = particles[i];
		}

		particle[0] = particle[1];
		particle[TRAIL_COUNT - 1] = particle[TRAIL_COUNT - 2];
	}
	gDeviceContext->Unmap(trail_buffer, 0);
}

void Render(float dt)
{
	XMFLOAT4 clear = normalize_color(0x93a9bcff);
	XMFLOAT4 blclear = normalize_color(0x000000ff);
	auto col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	col.w = 1.0f;
	XMFLOAT4 bclear = normalize_color(ImGui::GetColorU32(col));

	gDeviceContext->ClearRenderTargetView(default_rtv, (float*)&clear);
	gDeviceContext->ClearRenderTargetView(hdr_rtv, (float*)&clear);
	gDeviceContext->ClearRenderTargetView(distort_rtv, (float*)&blclear);
	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, (float*)&col);
	gDeviceContext->ClearDepthStencilView(gDepthbufferDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);


	auto vp = viewport;
	//vp.TopLeftX = 0;
	//vp.TopLeftY = 0;

	if (viewport_render) {
		SetViewport();

		RenderPlane();
		RenderParticles();
		gDeviceContext->RSSetViewports(1, &vp);

		RenderComposite();
	}

	ImGui::Render();


}

}