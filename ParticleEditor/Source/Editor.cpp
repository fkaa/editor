#include "Editor.h"

#include <Windows.h>

#include <vector>
#include <algorithm>

#include "External/DirectXTK.h"
#include "External/Helpers.h"
#include "External/dxerr.h"

#include <ImwWindow.h>
#include <ImwWindowManagerDX11.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "External\IconsMaterialDesign.h"

#include "ParticleSystem.h"

#include "Camera.h"
#include "Particle.h"
#include "Output.h"
#include "Viewport.h"

inline float RandomFloat(float lo, float hi)
{
	return ((hi - lo) * ((float)rand() / RAND_MAX)) + lo;
}

class MyMenu : public ImwMenu
{
public:
	MyMenu()
		: ImwMenu(-1)
	{
	}

	virtual void OnMenu() override
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Show content", NULL, ImWindow::ImwWindowManager::GetInstance()->GetMainPlatformWindow()->IsShowContent()))
			{
				ImWindow::ImwWindowManager::GetInstance()->GetMainPlatformWindow()->SetShowContent(!ImWindow::ImwWindowManager::GetInstance()->GetMainPlatformWindow()->IsShowContent());
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "ALT-F4"))
			{
				ImWindow::ImwWindowManager::GetInstance()->Destroy();
			}

			ImGui::EndMenu();
		}
	}
};

class MyToolBar : public ImwToolBar {
public:
	MyToolBar()
	{
	}

	virtual void OnToolBar() override
	{
		ImGui::BeginChild("Toolbar.main", ImVec2(0, ImGui::GetItemsLineHeightWithSpacing() + 10), true);
		ImGui::Button(ICON_MD_PHOTO_FILTER " [FX]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Particle FX");

		ImGui::SameLine();
		ImGui::TextDisabled("|");

		ImGui::SameLine();
		ImGui::Button(ICON_MD_GRAIN " [Trail]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Particle Trail");

		ImGui::SameLine();
		ImGui::Button(ICON_MD_CROP_FREE " [Billboard]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Billboard Particle");

		ImGui::SameLine();
		ImGui::Button(ICON_MD_FILTER_HDR " [Geometry]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Geometry Particle");

		ImGui::EndChild();
	}
};

class MyStatusBar : public ImwStatusBar
{
public:
	MyStatusBar()
	{
	}

	virtual void OnStatusBar()
	{
		if (Editor::UnsavedChanges)
			ImGui::TextColored(ImVec4(1.f, 0.659f, 0.561f, 1.f), ICON_MD_WARNING " [Unsaved Changes]");
	}

};

class AttributeEditor : public ImwWindow {
public:
	AttributeEditor()
	{
		SetTitle(ICON_MD_MODE_EDIT " Attributes");
	}

	virtual void OnGui() override
	{

	}
};

class EffectTimeline : public ImwWindow {
public:
	EffectTimeline()
	{
		SetTitle(ICON_MD_ART_TRACK " Timeline");
	}

	virtual void OnGui() override
	{

	}
};

class TextureList : public ImwWindow {
public:
	TextureList()
	{
		SetTitle(ICON_MD_PERM_MEDIA " Textures");
	}

	virtual void OnGui()
	{
		static int selected = 0;
		ImGui::BeginChild("TextureList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < MAX_MATERIAL_TEXTURES; i++)
		{
			auto &tex = Editor::MaterialTextures[i];
			if (tex.m_TexturePath.empty())
				continue;
			char label[128];
			sprintf(label, "%s t%d %s", tex.m_SRV ? ICON_MD_DONE : ICON_MD_WARNING, i, tex.m_TextureName.c_str());
			if (ImGui::Selectable(label, selected == i))
				selected = i;
		}
		ImGui::EndChild();
		ImGui::Button(ICON_MD_ADD_A_PHOTO "");
		ImGui::SameLine();
		ImGui::Button(ICON_MD_DELETE "");

	}
};

class MaterialList : public ImwWindow {
public:
	MaterialList()
	{
		SetTitle(ICON_MD_LIBRARY_BOOKS " Materials");
	}

	virtual void OnGui()
	{
		static int selected = 0;
		ImGui::BeginChild("MaterialList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < MAX_TRAIL_MATERIALS; i++)
		{
			auto &mat = Editor::TrailMaterials[i];
			if (mat.m_ShaderPath.empty())
				continue;
			char label[128];
			sprintf(label, "%s %s", mat.m_PixelShader ? ICON_MD_DONE : ICON_MD_WARNING, mat.m_MaterialName.c_str());
			if (ImGui::Selectable(label, selected == i))
				selected = i;
		}
		ImGui::EndChild();
		ImGui::Button(ICON_MD_LIBRARY_ADD "");
		ImGui::SameLine();
		ImGui::Button(ICON_MD_DELETE "");

	}
};

class EffectList : public ImwWindow {
public:
	EffectList()
	{
		SetTitle(ICON_MD_LIBRARY_BOOKS " Effects");
	}

	virtual void OnGui()
	{
		static int selected = 0;
		ImGui::BeginChild("EffectList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < Editor::EffectDefinitions.size(); i++)
		{
			auto &fx = Editor::EffectDefinitions[i];
			
			char label[128];
			sprintf(label, ICON_MD_PHOTO_FILTER " %s [%d]", fx.name, fx.m_Count);
			if (ImGui::Selectable(label, selected == i)) {
				Editor::SelectedEffect = &Editor::EffectDefinitions[i];
			}
		}
		ImGui::EndChild();
		ImGui::Button(ICON_MD_LIBRARY_ADD "");
		ImGui::SameLine();
		ImGui::Button(ICON_MD_DELETE "");

	}
};

ParticleSystem *FXSystem;

namespace Editor {;

bool UnsavedChanges = false;
MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];

std::vector<ParticleEffect> EffectDefinitions;
ParticleEffect *SelectedEffect;

Output *ConsoleOutput;

void Reload(ID3D11Device *device)
{
	int texcount = 0;
	for (int i = 0; i < MAX_MATERIAL_TEXTURES; i++) {
		auto &mat = MaterialTextures[i];
		
		if (!mat.m_TexturePath.empty()) {
			//if (mat.m_SRV) {
				ID3D11ShaderResourceView *newtex = nullptr;

				std::wstring file = ConvertToWString(mat.m_TexturePath);
				auto res = CreateWICTextureFromFile(device, file.c_str(), (ID3D11Resource**)nullptr, &newtex);
				if (!SUCCEEDED(res)) {
					char err[256];
					WinErrorMsg(res, err, 256);

					ConsoleOutput->AddLog("[error] Failed to load texture '%s'\n", mat.m_TexturePath.c_str());
					ConsoleOutput->AddLog("[error] %s", err);
					continue;
				}
				if (mat.m_SRV)
					mat.m_SRV->Release();
				mat.m_SRV = newtex;
			//}
			texcount++;
		}
	}

	int trailmatcount = 0;
	for (int i = 0; i < MAX_TRAIL_MATERIALS; i++) {
		auto &mat = TrailMaterials[i];

		if (!mat.m_ShaderPath.empty()) {
			//if (mat.m_PixelShader) {
				ID3D11PixelShader *newps = nullptr;
				ID3DBlob *blob = nullptr;
				ID3DBlob *error = nullptr;

				std::wstring file = ConvertToWString(mat.m_ShaderPath);
				auto res = D3DCompileFromFile(
					file.c_str(),
					nullptr,
					D3D_COMPILE_STANDARD_FILE_INCLUDE,
					"PS",
					"ps_5_0",
					D3DCOMPILE_DEBUG,
					0,
					&blob,
					&error
				);
				if (!SUCCEEDED(res)) {
					ConsoleOutput->AddLog("[error] Failed to compile shader '%s'\n", mat.m_ShaderPath.c_str());

					if (error) {
						ConsoleOutput->AddLog("[error] %s", (char*)error->GetBufferPointer());
					} else {
						char err[256];
						WinErrorMsg(res, err, 256);
						ConsoleOutput->AddLog("[error] %s", err);
					}

					continue;
				}

				if (!SUCCEEDED(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &newps))) {
					ConsoleOutput->AddLog("[error] Failed to create pixel shader '%s'\n", mat.m_ShaderPath.c_str());
					continue;
				}

				if (mat.m_PixelShader)
					mat.m_PixelShader->Release();
				mat.m_PixelShader = newps;
			//}
			trailmatcount++;
		}
	}

	ConsoleOutput->AddLog("Reloaded %d textures and %d materials\n", texcount, trailmatcount);
}

void Style();

void Run()
{
	TrailMaterials[0].m_MaterialName = "Default";
	TrailMaterials[0].m_ShaderPath = "Resources/Shaders/BillboardParticleSimple.hlsl";
	MaterialTextures[0].m_TextureName = "DefaultChecker";
	MaterialTextures[0].m_TexturePath = "Resources/Textures/Plane.dds";
	MaterialTextures[1].m_TextureName = "Noise";
	MaterialTextures[1].m_TexturePath = "Resources/Textures/Noise.png";

	BillboardParticleDefinition def = {};
	def.lifetime = -1.f;
	def.m_Material = &TrailMaterials[0];

	ParticleEffect fx = {};
	fx.m_Count = 1;
	fx.m_Entries[0].type = ParticleType::Billboard;
	fx.m_Entries[0].billboard = &def;
	EffectDefinitions.push_back(fx);




	ImWindow::ImwWindowManagerDX11 manager;
	manager.Init();
	manager.SetMainTitle("Editor");

	Style();

	new MyMenu();
	new MyToolBar();
	new MyStatusBar();
	ImwWindow *viewport = new EditorViewport();
	ImwWindow *attr = new AttributeEditor();
	ImwWindow *timeline = new EffectTimeline();
	ImwWindow *textures = new TextureList();
	ImwWindow *materials = new MaterialList();
	ImwWindow *effects = new EffectList();
	ConsoleOutput = new Output();

	ConsoleOutput->AddLog("Particle Editor v0.1\n");
	ConsoleOutput->AddLog("LMB to rotate, RMB to drag\n");
	ConsoleOutput->AddLog("Ctrl-R to reload resources\n");

	manager.Dock(viewport, E_DOCK_ORIENTATION_CENTER);
	manager.Dock(textures, E_DOCK_ORIENTATION_RIGHT, 0.2f);
	manager.DockWith(materials, textures, E_DOCK_ORIENTATION_CENTER);
	manager.DockWith(attr, materials, E_DOCK_ORIENTATION_BOTTOM, 0.5f);

	manager.Dock(effects, E_DOCK_ORIENTATION_LEFT, 0.15);

	manager.Dock(timeline, E_DOCK_ORIENTATION_BOTTOM, 0.2f);
	manager.DockWith(ConsoleOutput, timeline, E_DOCK_ORIENTATION_CENTER);

	bool reloading = false;
	Reload(ImwPlatformWindowDX11::s_pDevice);

	do
	{
		auto &io = ImGui::GetIO();

		if (io.KeysDown[82] && io.KeysDown[17]) {
			if (!reloading) {
				Reload(ImwPlatformWindowDX11::s_pDevice);
				reloading = true;
			}
		} else {
			reloading = false;
		}

		Sleep(1);
	} while (manager.Run(false) && manager.Run(true));
	
}

void Style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	auto accent = ImVec4(0.529f, 0.392f, 0.722f, 1.00f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.19f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = accent;
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

}
