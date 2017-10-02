#include "ParticleSystem.h"

#include <d3d11.h>
#include <algorithm>

#include "External/dxerr.h"
#include "External/Helpers.h"
#include "External/DirectXTK.h"
#include "Editor.h"

inline float RandomFloat(float lo, float hi)
{
	return ((hi - lo) * ((float)rand() / RAND_MAX)) + lo;
}

ParticleSystem::ParticleSystem(const wchar_t *file, UINT capacity, UINT width, UINT height, ID3D11Device *device, ID3D11DeviceContext *cxt)
	: capacity(capacity), device(device), cxt(cxt)
{
	//if (file)
	//	DeserializeParticles(file, effect_definitions, particle_definitions);

	ID3DBlob *blob = compile_shader(L"Resources/Shaders/BillboardParticleSimple.hlsl", "VS", "vs_5_0", device);
	DXCALL(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DefaultBillboardVS));

	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "IDX",      0, DXGI_FORMAT_R32_SINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_DefaultBillboardLayout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob->GetBufferPointer(), blob->GetBufferSize(), device);

	blob = compile_shader(L"Resources/Shaders/BillboardParticleSimple.hlsl", "GS", "gs_5_0", device);
	DXCALL(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DefaultBillboardGS));

	m_BillboardParticles.reserve(capacity);
	m_BillboardBuffer = new VertexBuffer<BillboardParticle>(device, BufferUsageDynamic, BufferAccessWrite, capacity);
}

ParticleSystem::~ParticleSystem()
{
	delete m_BillboardBuffer;
}

void ParticleSystem::ProcessFX(ParticleEffect *fx, XMMATRIX model, float dt)
{
	for (int i = 0; i < fx->m_Count; i++) {
		auto &entry = fx->m_Entries[i];

		fx->age += 0.01f;

		switch (entry.type) {
			case ParticleType::Billboard:
			{
				auto def = *entry.billboard;
				
				auto particle = BillboardParticle{
					{0, 0, 0},
					{1, 1},
					fx->age,
					(int)(Editor::TrailMaterials - def.m_Material)
				};

				m_BillboardParticles.push_back(particle);
			} break;
		}
	}
}

void ParticleSystem::ProcessFX(ParticleEffect & fx, XMMATRIX model, XMVECTOR velocity, float dt)
{

}

void ParticleSystem::AddFX(std::string name, XMMATRIX model)
{
	ParticleEffectInstance effect = {
		XMVectorAdd({0, 0.05f, 0 }, XMVector3Transform({}, model)),
		GetFX(name)
	};
	m_ParticleEffects.push_back(effect);
}

ParticleEffect ParticleSystem::GetFX(std::string name)
{
	auto result = std::find_if(m_ParticleEffectDefinitions.begin(), m_ParticleEffectDefinitions.end(), [name](ParticleEffect &a) {
		return std::strcmp(name.c_str(), a.name) == 0;
	});

	return *result;
}

void ParticleSystem::update(Camera *cam, float dt)
{
	//for (auto &effect : m_ParticleEffects) 

	BillboardParticle *ptr = m_BillboardBuffer->Map(cxt);
	for (auto particle : m_BillboardParticles) {
		*ptr++ = particle;
	}
	m_BillboardBuffer->Unmap(cxt);

}

void ParticleSystem::frame()
{
	m_BillboardParticles.clear();
}

void ParticleSystem::render(Camera *cam, CommonStates *states, ID3D11RenderTargetView *dst_rtv)
{
	{
		auto stride = (UINT)sizeof(BillboardParticle);
		auto offset = 0u;

		cxt->IASetInputLayout(m_DefaultBillboardLayout);
		cxt->IASetVertexBuffers(0, 1, *m_BillboardBuffer, &stride, &offset);
		cxt->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		cxt->VSSetShader(m_DefaultBillboardVS, nullptr, 0);
		cxt->GSSetShader(m_DefaultBillboardGS, nullptr, 0);

		cxt->GSSetConstantBuffers(0, 1, *cam->GetBuffer());

		//cxt->PSSetShaderResources(0, 1, &m_ParticleAtlas);

		ID3D11SamplerState *samplers[] = {
			states->LinearClamp(),
			states->LinearWrap(),
			states->PointClamp(),
			states->PointWrap()
		};
		cxt->PSSetSamplers(0, 4, samplers);


		for (int i = 0; i < m_BillboardParticles.size(); i++) {
			cxt->PSSetShader(Editor::TrailMaterials[m_BillboardParticles[i].idx].m_PixelShader, nullptr, 0);
			cxt->Draw(1, i);
		}

		cxt->GSSetShader(nullptr, nullptr, 0);
	}
}
