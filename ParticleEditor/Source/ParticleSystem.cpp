#include "ParticleSystem.h"

#include <d3d11.h>
#include <algorithm>
#include <fstream>

#include "External/dxerr.h"
#include "External/Helpers.h"
#include "External/DirectXTK.h"
#include "Editor.h"


ParticleSystem::ParticleSystem(const wchar_t *file, UINT capacity, UINT width, UINT height, ID3D11Device *device, ID3D11DeviceContext *cxt)
	: capacity(capacity), device(device), cxt(cxt)
{
	//if (file)
	//	DeserializeParticles(file, effect_definitions, particle_definitions);

	D3D11_BLEND_DESC state = {};
	ZeroMemory(&state, sizeof(D3D11_BLEND_DESC));
	state.RenderTarget[0].BlendEnable = TRUE;
	state.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	state.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	state.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	state.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	state.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	state.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	state.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DXCALL(device->CreateBlendState(&state, &m_ParticleBlend));

	ID3DBlob *blob = compile_shader(L"Resources/Shaders/BillboardParticleSimple.hlsl", "VS", "vs_5_0", device);
	DXCALL(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DefaultBillboardVS));

	D3D11_INPUT_ELEMENT_DESC billboard_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "IDX",      0, DXGI_FORMAT_R32_SINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_DefaultBillboardLayout = create_input_layout(billboard_desc, ARRAYSIZE(billboard_desc), blob->GetBufferPointer(), blob->GetBufferSize(), device);

	blob = compile_shader(L"Resources/Shaders/BillboardParticleSimple.hlsl", "GS", "gs_5_0", device);
	DXCALL(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DefaultBillboardGS));

	m_BillboardParticles.reserve(capacity);
	m_BillboardBuffer = new VertexBuffer<BillboardParticle>(device, BufferUsageDynamic, BufferAccessWrite, capacity);

	m_TrailBuffer = new VertexBuffer<TrailParticle>(device, BufferUsageDynamic, BufferAccessWrite, TRAIL_COUNT * TRAIL_PARTICLE_COUNT);
	//m_TrailParticles.push_back({});

	blob = compile_shader(L"Resources/Shaders/TrailParticleSimple.hlsl", "VS", "vs_5_0", device);
	DXCALL(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_vs));

	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	trail_layout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob->GetBufferPointer(), blob->GetBufferSize(), device);

	blob = compile_shader(L"Resources/Shaders/TrailParticleSimple.hlsl", "GS", "gs_5_0", device);
	DXCALL(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_gs));

	blob = compile_shader(L"Resources/Shaders/TrailParticleSimple.hlsl", "PS", "ps_5_0", device);
	DXCALL(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &trail_ps));

	ReadSphereModel();


	//m_GeometryParticles.reserve(capacity);

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
					(int)(def.m_Material - Editor::TrailMaterials)
				};

				m_BillboardParticles.push_back(particle);
			} break;
			case ParticleType::Geometry: {
				auto def = *entry.geometry;

				auto particle = GeometryParticle{
					XMMatrixTranslation(0, 0, 0),
					fx->age,
					(int)(def.m_Material - Editor::TrailMaterials)
				};

				m_GeometryParticles.push_back(particle);
			} break;
			case ParticleType::Trail: {
				auto &trailfx = entry.trail;
				auto def = *trailfx.def;

				if (trailfx.trailidx == -1) {
					trailfx.trailidx = m_TrailParticles.size();
					
					m_TrailParticles.push_back({});
					m_TrailParticles[trailfx.trailidx].def = trailfx.def;
					m_TrailParticles[trailfx.trailidx].idx = (int)(def.m_Material - Editor::TrailMaterials);
				}

				auto &trail = m_TrailParticles[trailfx.trailidx];
				auto particles = trail.points;
				
				if (trail.dead >= TRAIL_COUNT - 2) {
					// remove
				}

				if (trail.spawn >= def.frequency) {
					particles[0] = {
						def.m_StartPosition.GetPosition(),// RandomFloat(-0.01, 0.01), 0, RandomFloat(-0.01, 0.01)),
						def.m_StartVelocity.GetVelocity(),
						SimpleMath::Vector2(0.13, 1.0)
					};
				}
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

	{
		BillboardParticle *ptr = m_BillboardBuffer->Map(cxt);
		for (auto particle : m_BillboardParticles) {
			*ptr++ = particle;
		}
		m_BillboardBuffer->Unmap(cxt);
	}

	{
		GeometryParticle *ptr = m_GeometryInstanceBuffer->Map(cxt);
		for (auto particle : m_GeometryParticles) {
			*ptr++ = particle;
		}
		m_GeometryInstanceBuffer->Unmap(cxt);
	}

	{
		for (auto &trail : m_TrailParticles) {
			auto particles = trail.points;
			auto &def = *trail.def;

			while (trail.spawn >= def.frequency) {
				trail.spawn -= def.frequency;

				if (trail.age >= def.lifetime) {

					if (trail.dead >= TRAIL_COUNT - 2) {
						// delete trail
					}
					else {
						trail.dead++;
					}
				}

				TrailParticle particle = particles[1];
				particle.m_Position += particle.m_Velocity;
				particle.m_Position.y += def.m_Gravity;

				for (int i = TRAIL_COUNT - 2; i > 1; i--) {
					auto p = particles[i - 1];
					p.m_Position += particle.m_Velocity * dt;
					p.m_Position.y += def.m_Gravity * dt;
					particles[i] = p;
				}

				for (int i = 0; i < trail.dead; i++) {
					particles[i] = particles[trail.dead];
				}

				particles[1] = particles[0];
				particles[TRAIL_COUNT - 1] = particles[TRAIL_COUNT - 2];
			}

			trail.age += dt;
			trail.spawn += dt;
		}

		TrailParticle *ptr = m_TrailBuffer->Map(cxt);
		for (auto particle : m_TrailParticles) {
			memcpy(ptr, particle.points, TRAIL_COUNT * sizeof(TrailParticle));
			ptr += TRAIL_COUNT * sizeof(TrailParticle);
		}
		m_TrailBuffer->Unmap(cxt);
	}
}

void ParticleSystem::frame()
{
	m_BillboardParticles.clear();
	m_GeometryParticles.clear();
}

void ParticleSystem::ReadSphereModel()
{
	std::ifstream meshfile("Resources/Mesh/sphere.dat", std::ifstream::in | std::ifstream::binary);
	if (!meshfile.is_open())
		throw "Can't load sky dome mesh";

	uint32_t vertexcount = 0;
	uint32_t indexcount = 0;

	meshfile.read(reinterpret_cast<char*>(&vertexcount), sizeof(uint32_t));
	if (!vertexcount)
		throw "Parse error";

	meshfile.read(reinterpret_cast<char*>(&indexcount), sizeof(uint32_t));
	if (!indexcount)
		throw "Parse error";

	std::vector<SphereVertex> vertices;
	std::vector<UINT16> indices;

	vertices.resize(vertexcount);
	meshfile.read(reinterpret_cast<char*>(&vertices.front()), sizeof(SphereVertex) * vertexcount);

	indices.resize(indexcount);
	meshfile.read(reinterpret_cast<char*>(&indices.front()), sizeof(UINT16) * indexcount);

	m_GeometryIndices = indexcount;
	m_GeometryBuffer = new VertexBuffer<SphereVertex>(device, BufferUsageImmutable, BufferAccessNone, vertexcount, &vertices[0]);
	m_GeometryIndexBuffer = new IndexBuffer<UINT16>(device, BufferUsageImmutable, BufferAccessNone, indexcount, &indices[0]);

	m_GeometryInstanceBuffer = new VertexBuffer<GeometryParticle>(device, BufferUsageDynamic, BufferAccessWrite, 128);

	ID3DBlob *blob = compile_shader(L"Resources/Shaders/GeometryParticleSimple.hlsl", "VS", "vs_5_0", device);
	DXCALL(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DefaultGeometryVS));

	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "MODEL",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 0 },
		{ "MODEL",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
		{ "MODEL",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
		{ "MODEL",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
		{ "AGE",      0, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
		{ "IDX",      0, DXGI_FORMAT_R32_SINT,           1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
	};
	m_DefaultGeometryLayout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob->GetBufferPointer(), blob->GetBufferSize(), device);


}

void ParticleSystem::render(Camera *cam, CommonStates *states, ID3D11DepthStencilView *dst_dsv, ID3D11RenderTargetView *dst_rtv)
{

	ID3D11SamplerState *samplers[] = {
		states->LinearClamp(),
		states->LinearWrap(),
		states->PointClamp(),
		states->PointWrap()
	};
	cxt->VSSetSamplers(0, 4, samplers);
	cxt->PSSetSamplers(0, 4, samplers);

	float blend[4] = { 1, 1, 1, 1 };
	cxt->OMSetBlendState(m_ParticleBlend, nullptr, 0xffffffff);

	// spheres
	{
		UINT strides[2] = {
			(UINT)sizeof(SphereVertex),
			(UINT)sizeof(GeometryParticle)
		};

		UINT offsets[2] = {
			0,
			0
		};

		ID3D11Buffer *buffers[] = {
			*m_GeometryBuffer,
			*m_GeometryInstanceBuffer
		};

		cxt->IASetInputLayout(m_DefaultGeometryLayout);
		cxt->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		cxt->IASetIndexBuffer(*m_GeometryIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		cxt->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		cxt->VSSetShader(m_DefaultGeometryVS, nullptr, 0);
		cxt->VSSetConstantBuffers(0, 1, *cam->GetBuffer());

		cxt->OMSetDepthStencilState(states->DepthDefault(), 0);
		cxt->OMSetRenderTargets(1, &dst_rtv, dst_dsv);
		cxt->RSSetState(states->CullClockwise());

		for (int i = 0; i < m_GeometryParticles.size(); i++) {
			cxt->PSSetShader(Editor::TrailMaterials[m_GeometryParticles[i].idx].m_PixelShader, nullptr, 0);
			cxt->DrawIndexedInstanced(m_GeometryIndices, 1, 0, 0, i);
		}
	}

	cxt->RSSetState(states->CullCounterClockwise());
	
	//Trail
	{
		UINT stride = sizeof(TrailParticle);
		UINT offset = 0;// 1 * sizeof(TrailParticle);

		cxt->IASetVertexBuffers(0, 1, *m_TrailBuffer, &stride, &offset);
		cxt->IASetInputLayout(trail_layout);
		cxt->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
		cxt->VSSetShader(trail_vs, nullptr, 0);
		cxt->GSSetShader(trail_gs, nullptr, 0);
		cxt->PSSetShader(trail_ps, nullptr, 0);

		for (int i = 0; i < m_TrailParticles.size(); i++) {
			cxt->PSSetShader(Editor::TrailMaterials[m_TrailParticles[i].idx].m_PixelShader, nullptr, 0);
			cxt->Draw(TRAIL_COUNT, i * TRAIL_COUNT);
		}

		cxt->GSSetShader(nullptr, nullptr, 0);
	}

	// billboards
	{
		auto stride = (UINT)sizeof(BillboardParticle);
		auto offset = 0u;

		cxt->IASetInputLayout(m_DefaultBillboardLayout);
		cxt->IASetVertexBuffers(0, 1, *m_BillboardBuffer, &stride, &offset);
		cxt->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		cxt->VSSetShader(m_DefaultBillboardVS, nullptr, 0);
		cxt->GSSetShader(m_DefaultBillboardGS, nullptr, 0);
		cxt->GSSetConstantBuffers(0, 1, *cam->GetBuffer());

		cxt->OMSetDepthStencilState(states->DepthRead(), 0);
		cxt->OMSetRenderTargets(1, &dst_rtv, nullptr);

		for (int i = 0; i < m_BillboardParticles.size(); i++) {
			cxt->PSSetShader(Editor::TrailMaterials[m_BillboardParticles[i].idx].m_PixelShader, nullptr, 0);
			cxt->Draw(1, i);
		}

		cxt->GSSetShader(nullptr, nullptr, 0);
	}
}
