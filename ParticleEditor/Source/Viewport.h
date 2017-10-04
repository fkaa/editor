#pragma once

#include <fstream>
#include <vector>

#include <ImwPlatformWindowDX11.h>

#include "External\DirectXTK.h"
#include "External\DebugDraw.h"
#include "External\Helpers.h"

using namespace DirectX;
using namespace ImWindow;

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 textureCoordinates;
};

class SkySphere {
public:
	SkySphere(ID3D11Device *device) {
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

		std::vector<Vertex> vertices;
		std::vector<UINT16> indices;

		vertices.resize(vertexcount);
		meshfile.read(reinterpret_cast<char*>(&vertices.front()), sizeof(Vertex) * vertexcount);

		indices.resize(indexcount);
		meshfile.read(reinterpret_cast<char*>(&indices.front()), sizeof(UINT16) * indexcount);

		m_IndexCount = indexcount;
		m_VertexBuffer = new VertexBuffer<Vertex>(device, BufferUsageImmutable, BufferAccessNone, vertexcount, &vertices[0]);
		m_IndexBuffer = new IndexBuffer<UINT16>(device, BufferUsageImmutable, BufferAccessNone, indexcount, &indices[0]);

		ID3DBlob *blob = compile_shader(L"Resources/Shaders/SkySphere.hlsl", "VS", "vs_5_0", device);
		DXCALL(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_VertexShader));

		D3D11_INPUT_ELEMENT_DESC input_desc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		m_InputLayout = create_input_layout(input_desc, ARRAYSIZE(input_desc), blob->GetBufferPointer(), blob->GetBufferSize(), device);

		blob = compile_shader(L"Resources/Shaders/SkySphere.hlsl", "PS", "ps_5_0", device);
		DXCALL(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_PixelShader));
	}
	~SkySphere() {
		delete m_VertexBuffer;
		delete m_IndexBuffer;

		m_InputLayout->Release();
		m_VertexShader->Release();
		m_PixelShader->Release();
	}

	void Draw(ID3D11DeviceContext *cxt, Camera *camera)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0u;

		cxt->IASetInputLayout(m_InputLayout);

		cxt->IASetIndexBuffer(*m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		cxt->IASetVertexBuffers(0, 1, *m_VertexBuffer, &stride, &offset);
		cxt->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		cxt->VSSetShader(m_VertexShader, nullptr, 0);
		cxt->VSSetConstantBuffers(0, 1, *camera->GetBuffer());

		cxt->PSSetShader(m_PixelShader, nullptr, 0);

		cxt->DrawIndexed(m_IndexCount, 0, 0);
	}

	VertexBuffer<Vertex> *m_VertexBuffer;
	IndexBuffer<UINT16> *m_IndexBuffer;


	UINT m_IndexCount;
	ID3D11InputLayout *m_InputLayout;

	ID3D11VertexShader *m_VertexShader;
	ID3D11PixelShader *m_PixelShader;
};

class EditorViewport : public ImwWindow {
	friend class ImwPlatformWindowDX11;
public:
	EditorViewport() :
		device(ImwPlatformWindowDX11::s_pDevice),
		cxt(ImwPlatformWindowDX11::s_pDeviceContext),
		m_Camera(new Camera(ImwPlatformWindowDX11::s_pDevice, 3.f)),
		m_Batch(new PrimitiveBatch<VertexPositionColor>(ImwPlatformWindowDX11::s_pDeviceContext)),
		m_Effect(new BasicEffect(ImwPlatformWindowDX11::s_pDevice)),
		m_States(new CommonStates(ImwPlatformWindowDX11::s_pDevice)),
		m_Sphere(ImwPlatformWindowDX11::s_pDevice),
		m_RenderSize({}),
		m_Dirty(true)
	{
		SetTitle(ICON_MD_VIDEOCAM " Viewport");
		m_Effect->SetVertexColorEnabled(true);

		{
			void const* bytecode;
			size_t len;
			m_Effect->GetVertexShaderBytecode(&bytecode, &len);

			m_BatchLayout = create_input_layout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount, bytecode, len, device);
		}

		{
			auto display = ImGui::GetIO().DisplaySize;
			OnResize(display.x, display.y);
		}

		FXSystem = new ParticleSystem(L"", 2048, 0, 0, device, cxt);
	}

	~EditorViewport() {
		delete m_Camera;
		delete m_Batch;
		delete m_Effect;
		delete m_States;
	}

	void OnResize(int width, int height)
	{


		if (m_DepthDSV)
			m_DepthDSV->Release();

		ID3D11Texture2D *texture;
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			desc.Format = DXGI_FORMAT_R32_TYPELESS;
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.SampleDesc.Count = 1;
			DXCALL(device->CreateTexture2D(&desc, nullptr, &texture));
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		DXCALL(device->CreateDepthStencilView(texture, &desc, &m_DepthDSV));

		texture->Release();
	}

	virtual bool IsCustomRendered() override
	{
		return true;
	}

	virtual void OnGui() override
	{
		auto min = GetLastPosition();
		auto max = GetLastSize();
		auto size = max;
		auto lo = min(size.x, size.y);
		auto hi = max(size.x, size.y);

		m_Camera->SetProjection(XMMatrixPerspectiveFovRH(45.f, float(hi) / float(lo), 0.1f, 100.f));
		
		// temp
		//OnResize(size.x, size.y);
		m_RenderSize = size;

		// if current frame's viewport size differs from previous frame, we need to
		// recreate our resources
		auto display = ImGui::GetIO().DisplaySize;
		if ((int)display.x != (int)m_DisplaySize.x &&
			(int)display.y != (int)m_DisplaySize.y)
		{
			OnResize(display.x, display.y);
			m_DisplaySize = display;
		}


		// if need to refresh and we dont have lmb down (resizing), recreate
		// resources dependent on viewport size
		if (m_Dirty) {

			m_RenderSize = size;
			m_Dirty = false;
		}

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = min.x;
		viewport.TopLeftY = min.y;
		viewport.Width = m_RenderSize.x;
		viewport.Height = m_RenderSize.y;
		viewport.MaxDepth = 1.f;

		cxt->OMSetBlendState(m_States->AlphaBlend(), nullptr, 0xFFFFFFFF);
		cxt->OMSetDepthStencilState(m_States->DepthNone(), 0);
		cxt->OMSetRenderTargets(1, &ImwPlatformWindowDX11::s_pRTV, nullptr);
		cxt->RSSetState(m_States->CullNone());
		cxt->RSSetViewports(1, &viewport);

		auto &io = ImGui::GetIO();

		//if ((1 << 15) & GetAsyncKeyState(VK_LMENU)) {
		if (io.MouseDown[0]) {
			auto drag = ImGui::GetIO().MouseDelta;

			m_Camera->Rotate(drag.x, drag.y);
		}
		else if (io.MouseDown[1]) {
			auto drag = ImGui::GetIO().MouseDelta;

			m_Camera->Pan(drag.x, drag.y);
		}
		//}
		m_Camera->Zoom(io.MouseWheel);

		m_Camera->Update(cxt);

		m_Sphere.Draw(cxt, m_Camera);

		m_Effect->SetProjection(m_Camera->GetProjection());
		m_Effect->SetView(m_Camera->GetView());
		m_Effect->Apply(cxt);

		cxt->IASetInputLayout(m_BatchLayout);

		m_Batch->Begin();

		DX::DrawGrid(m_Batch, { 5, 0, 0 }, { 0, 0, 5 }, { 0, 0, 0 }, 10, 10, Colors::SlateGray, Colors::MediumAquamarine);

		BoundingBox box;
		box.Center = { 0, 0, 0 };
		box.Extents = { 0.5f, 0.5f, 0.5f };
		DX::Draw(m_Batch, box, Colors::MediumAquamarine);

		m_Batch->End();
		
		ID3D11ShaderResourceView *SRVs[MAX_MATERIAL_TEXTURES] = {};
		for (int i = 0; i < MAX_MATERIAL_TEXTURES; i++) {
			auto &mat = Editor::MaterialTextures[i];
			SRVs[i] = mat.m_SRV;
		}
		cxt->VSSetShaderResources(0, MAX_MATERIAL_TEXTURES, SRVs);
		cxt->PSSetShaderResources(0, MAX_MATERIAL_TEXTURES, SRVs);
		cxt->OMSetBlendState(m_States->AlphaBlend(), nullptr, 0xFFFFFFFF);

		if (Editor::SelectedEffect) {
			FXSystem->ProcessFX(Editor::SelectedEffect, XMMatrixTranslation(0, 0, 0), 1.f);
		}

		cxt->ClearDepthStencilView(m_DepthDSV, D3D11_CLEAR_DEPTH, 1.f, 0);

		FXSystem->update(m_Camera, 1.f);
		FXSystem->render(m_Camera, m_States, m_DepthDSV, ImwPlatformWindowDX11::s_pRTV);
		FXSystem->frame();
	}

private:
	ID3D11Device *device;
	ID3D11DeviceContext *cxt;

	Camera *m_Camera;

	ID3D11DepthStencilView *m_DepthDSV;
	ID3D11InputLayout *m_BatchLayout;
	PrimitiveBatch<VertexPositionColor> *m_Batch;
	BasicEffect *m_Effect;
	CommonStates *m_States;

	SkySphere m_Sphere;

	ImVec2 m_RenderSize;
	ImVec2 m_DisplaySize;
	bool m_Dirty;
};
