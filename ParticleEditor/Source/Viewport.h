#pragma once

#include <fstream>
#include <vector>

#include <ImwPlatformWindowDX11.h>

#include "External\DirectXTK.h"
#include "External\DebugDraw.h"
#include "External\Helpers.h"
#include "External\ImGuizmo.h"

#include <imgui_internal.h>

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
		m_Mouse(new Mouse()),
		m_Keyboard(new Keyboard()),
		m_RenderSize({}),
		m_Dirty(true),
		m_GizmoActive(false),
		m_Dragging(false),
		m_AutoRotate(false)
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

		XMStoreFloat4x4(&m_ParticlePosition, XMMatrixTranslation(0, 0, 0));

		FXSystem = new ParticleSystem(L"", 2048, 0, 0, device, cxt);
	}

	~EditorViewport() {
		delete m_Camera;
		delete m_Batch;
		delete m_Effect;
		delete m_States;
		delete m_Keyboard;
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
		auto mstate = m_Mouse->GetState();
		m_MTracker.Update(mstate);
		auto state = m_Keyboard->GetState();
		m_Tracker.Update(state);

		if (m_Tracker.pressed.G)
			m_GizmoActive = !m_GizmoActive;

		if (m_Tracker.pressed.P)
			Editor::Paused = !Editor::Paused;

		if (m_Tracker.pressed.R)
			m_AutoRotate = !m_AutoRotate;

		auto min = GetLastPosition();
		auto max = GetLastSize();
		auto size = max;
		auto lo = min(size.x, size.y);
		auto hi = max(size.x, size.y);

		auto delta = ImGui::GetIO().DeltaTime;

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

		ImGuizmo::SetRect(min.x, min.y, m_RenderSize.x, m_RenderSize.y);

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

		bool inside = ImGui::IsMouseHoveringWindow();
		bool shiftdown = io.KeyShift;
		//if ((1 << 15) & GetAsyncKeyState(VK_LMENU)) {
		bool gizmo = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

		if (m_AutoRotate) {
			m_Camera->Rotate(delta * 15.f, 0.f);
		}
		
		
		if (!gizmo) {
			if (io.MouseDown[0]) {
				auto drag = ImGui::GetIO().MouseDelta;

				if (m_MTracker.leftButton == Mouse::ButtonStateTracker::ButtonState::PRESSED && inside) {
					m_Dragging = true;
				}

				if (m_Dragging)
					m_Camera->Rotate(drag.x * (shiftdown ? 5 : 2.5f), drag.y * (shiftdown ? 5 : 2.5f));
			}
			else if (io.MouseDown[1]) {
				auto drag = ImGui::GetIO().MouseDelta;

				if (m_MTracker.rightButton == Mouse::ButtonStateTracker::ButtonState::PRESSED && inside) {
					m_Dragging = true;
				}

				if (m_Dragging)
					m_Camera->Pan(drag.x * (shiftdown ? 5 : 2.5f), drag.y * (shiftdown ? 5 : 2.5f));
			}
			else {
				m_Dragging = false;
			}
		}
		//}

		if (!gizmo && inside)
			m_Camera->Zoom(io.MouseWheel * (shiftdown ? 25 : 10));

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
		//DX::Draw(m_Batch, box, Colors::MediumAquamarine);

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
			XMFLOAT4X4 view, proj;

			XMStoreFloat4x4(&view, m_Camera->GetView());
			XMStoreFloat4x4(&proj, m_Camera->GetProjection());

			if (m_GizmoActive) {
				ImGuizmo::Enable(!m_Dragging && gizmo);
				ImGuizmo::Manipulate((float*)view.m, (float*)proj.m, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, (float*)m_ParticlePosition.m, nullptr, nullptr, nullptr, nullptr);
			}
			auto pos = XMLoadFloat4x4(&m_ParticlePosition);

			FXSystem->ProcessFX(Editor::SelectedEffect, pos, delta * Editor::Speed * (Editor::Paused ? 0.f : 1.f));
		


			if (Editor::SelectedEffect->age >= Editor::SelectedEffect->time) {
				Editor::SelectedEffect->age = 0;
			}
		}

		cxt->ClearDepthStencilView(m_DepthDSV, D3D11_CLEAR_DEPTH, 1.f, 0);

		FXSystem->update(m_Camera, delta * Editor::Speed * (Editor::Paused ? 0.f : 1.f));
		FXSystem->render(m_Camera, m_States, m_DepthDSV, ImwPlatformWindowDX11::s_pRTV, Editor::Debug);
		FXSystem->frame();

		ImVec2 window_pos = ImGui::GetWindowPos() + ImVec2(10, 10);
		ImGui::SetNextWindowPos(window_pos, ImGuiSetCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
		if (Editor::SelectedEffect) {
			if (ImGui::Begin("Viewport:", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::TextColored(FX_COLORS[0], FX_ICON " %s", Editor::SelectedEffect->name);
				char label[128];
				for (int j = 0; j < Editor::SelectedEffect->m_Count; j++) {
					auto &entry = Editor::SelectedEffect->m_Entries[j];

					switch (entry.type) {
						case ParticleType::Billboard: {
							auto def = entry.billboard;
							sprintf(label, ICON_MD_SUBDIRECTORY_ARROW_RIGHT " " BILLBOARD_ICON " %s", def->name.c_str());
							ImGui::TextColored(BILLBOARD_COLORS[0], " %s", label);
						} break;
						case ParticleType::Geometry: {
							auto def = entry.geometry;
							sprintf(label, ICON_MD_SUBDIRECTORY_ARROW_RIGHT " " GEOMETRY_ICON " %s", def->name.c_str());
							ImGui::TextColored(GEOMETRY_COLORS[0], " %s", label);
						} break;
						case ParticleType::Trail: {
							auto def = entry.trail.def;
							sprintf(label, ICON_MD_SUBDIRECTORY_ARROW_RIGHT " " TRAIL_ICON " %s", def->name.c_str());
							ImGui::Text(" %s", label);
						} break;
					}
				}
				ImGui::End();
			}
		}
		ImGui::PopStyleColor();
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
	Mouse *m_Mouse;
	Mouse::ButtonStateTracker m_MTracker;

	Keyboard *m_Keyboard;
	Keyboard::KeyboardStateTracker m_Tracker;


	SkySphere m_Sphere;

	XMFLOAT4X4 m_ParticlePosition;
	ImVec2 m_RenderSize;
	ImVec2 m_DisplaySize;
	bool m_Dirty;
	bool m_GizmoActive;
	bool m_Dragging;
	bool m_AutoRotate;
};
