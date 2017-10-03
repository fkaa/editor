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

#include <External/json.hpp>

using json = nlohmann::json;

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
		
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.420, 0.482, 0.082, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.576, 0.647, 0.216, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.741, 0.808, 0.404, 1.0f));
		ImGui::Button(ICON_MD_PHOTO_FILTER " [FX]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Particle FX");
		ImGui::PopStyleColor(3);

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

void MaterialCombo(int *idx)
{
	std::vector<const char *> names;
	for (int i = 0; i < MAX_TRAIL_MATERIALS; i++) {
		TrailParticleMaterial &mat = Editor::TrailMaterials[i];
		if (mat.m_MaterialName.empty())
			continue;

		names.push_back(mat.m_MaterialName.c_str());
	}

	ImGui::Combo("Material", idx, (const char**)names.data(), names.size());
}

class AttributeEditor : public ImwWindow {
public:
	AttributeEditor()
	{
		SetTitle(ICON_MD_MODE_EDIT " Attributes");
	}

	virtual void OnGui() override
	{
		switch (Editor::SelectedObject.type) {
			case Editor::AttributeType::Texture: {
				MaterialTexture &tex = Editor::MaterialTextures[Editor::SelectedObject.index];
				ImGui::Text("Texture");

				if (tex.m_SRV)
					ImGui::Image(tex.m_SRV, ImVec2(128, 128));

				tex.m_TextureName.resize(128, '\0');
				ImGui::InputText("Name", (char*)tex.m_TextureName.data(), 120);
				tex.m_TexturePath.resize(128, '\0');
				ImGui::InputText("Path", (char*)tex.m_TexturePath.data(), 120);

			} break;
			case Editor::AttributeType::Material: {
				TrailParticleMaterial &mat = Editor::TrailMaterials[Editor::SelectedObject.index];
				ImGui::Text("Material");

				mat.m_MaterialName.resize(128, '\0');
				ImGui::InputText("Name", (char*)mat.m_MaterialName.data(), 120);
				mat.m_ShaderPath.resize(128, '\0');
				ImGui::InputText("Path", (char*)mat.m_ShaderPath.data(), 120);

			} break;
			case Editor::AttributeType::Billboard: {
				BillboardParticleDefinition &def = Editor::BillboardDefinitions[Editor::SelectedObject.index];
				ImGui::Text("Billboard");

				def.name.resize(128, '\0');
				ImGui::InputText("Name", (char*)def.name.data(), 120);

				int idx = def.m_Material - Editor::TrailMaterials;
				MaterialCombo(&idx);

				def.m_Material = &Editor::TrailMaterials[idx];
			} break;
			case Editor::AttributeType::None:
				ImGui::Text("No selection");
				break;
		}
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
			if (ImGui::Selectable(label, selected == i)) {
				Editor::SelectedObject = {
					Editor::AttributeType::Texture,
					i
				};

				selected = i;
			}
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
			if (ImGui::Selectable(label, selected == i)) {
				Editor::SelectedObject = {
					Editor::AttributeType::Material,
					i
				};
				selected = i;
			}
		}
		ImGui::EndChild();
		ImGui::Button(ICON_MD_LIBRARY_ADD "");
		ImGui::SameLine();
		ImGui::Button(ICON_MD_DELETE "");

	}
};

class ParticleList : public ImwWindow {
public:
	ParticleList()
	{
		SetTitle(ICON_MD_LIBRARY_BOOKS " Particles");
	}

	virtual void OnGui()
	{
		static int selected = 0;
		ImGui::BeginChild("ParticleList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
		for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++)
		{
			auto &def = Editor::BillboardDefinitions[i];
			if (def.name.empty())
				continue;

			char label[128];
			sprintf(label, ICON_MD_CROP_FREE " %s", def.name.c_str());
			if (ImGui::Selectable(label, selected == i)) {
				Editor::SelectedObject = {
					Editor::AttributeType::Billboard,
					i
				};
			}
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
			bool node = ImGui::TreeNode(label);
			
			ImGui::SameLine();
			
			if (ImGui::Button(ICON_MD_PLAY_ARROW)) {
				Editor::SelectedObject = Editor::AttributeObject{
					Editor::AttributeType::Effect,
					i
				};
				Editor::SelectedEffect = &Editor::EffectDefinitions[i];
			}

			if (node) {
				for (int j = 0; j < fx.m_Count; j++) {
					auto &entry = fx.m_Entries[j];

					switch (entry.type) {
						case ParticleType::Billboard: {
							auto def = entry.billboard;
							sprintf(label, ICON_MD_CROP_FREE " %s", def->name.c_str());
							if (ImGui::Selectable(label, selected == j)) {
								Editor::SelectedObject = {
									Editor::AttributeType::Billboard,
									(int)(def - Editor::BillboardDefinitions)
								};
							}
						} break;
					}
				}
				ImGui::TreePop();
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

Output *ConsoleOutput;

bool UnsavedChanges = true;
AttributeObject SelectedObject;

MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];
BillboardParticleDefinition BillboardDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];

std::vector<ParticleEffect> EffectDefinitions;

ParticleEffect *SelectedEffect;

JsonValue EditorState;

BillboardParticleDefinition *GetBillboardDef(std::string name)
{
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto &def = BillboardDefinitions[i];
		if (def.name == name)
			return &BillboardDefinitions[i];
	}

	return nullptr;
}

TrailParticleMaterial *GetMaterial(std::string name)
{
	for (int i = 0; i < MAX_TRAIL_MATERIALS; i++) {
		auto &def = TrailMaterials[i];
		if (def.m_MaterialName == name)
			return &TrailMaterials[i];
	}

	return nullptr;
}

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

void Load()
{
	std::ifstream i("editor.json");
	json data;
	i >> data;
	
	auto materials = data["materials"];
	auto mats = TrailMaterials;
	for (auto entry : materials) {
		auto mat = TrailParticleMaterial {
			entry["name"],
			entry["path"],
			nullptr
		};
		*mats++ = mat;
	}

	auto textures = data.at("textures");
	auto texs = MaterialTextures;
	for (auto entry : textures) {
		auto tex = MaterialTexture{
			entry["name"],
			entry["path"],
			nullptr
		};
		*texs++ = tex;
	}

	auto bdefs = data.at("billboard_definitions");
	auto bd = BillboardDefinitions;
	for (auto entry : bdefs) {
		BillboardParticleDefinition def = {};
		std::string n = entry["name"];
		def.name = n;
		def.m_Material = GetMaterial(entry["material_name"]);
		def.lifetime = entry["lifetime"];
		*bd++ = def;
	}

	auto effects = data.at("fx");
	for (auto entry : effects) {
		
		ParticleEffect fx = {};
		std::string name = entry["name"];
		memcpy(fx.name, name.data(), min(name.size(), 15));

		auto entries = entry["entries"];
		for (auto fxentry : entries) {
			auto type = ParticleTypeFromString(fxentry["type"]);
			std::string name = fxentry["name"];
			
			ParticleEffectEntry ent = {};
			ent.type = type;

			switch (type) {
				case ParticleType::Billboard:
					ent.billboard = GetBillboardDef(name);
					break;
			}

			fx.m_Entries[fx.m_Count++] = ent;
		}

		EffectDefinitions.push_back(fx);
	}
}

void Save() {
	json j;

	j["billboard_definitions"] = {};
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto def = BillboardDefinitions[i];
		if (def.name.empty())
			continue;

		json definition = {
			{"name", def.name},
			{"material_name", def.m_Material->m_MaterialName},
			{"lifetime", def.lifetime },
			/*{"start",
				{
					"min", {def.m_StartPosition.m_Min.x, def.m_StartPosition.m_Min.y, def.m_StartPosition.m_Min.z }
				},
				{
					"max", {def.m_StartPosition.m_Max.x, def.m_StartPosition.m_Max.y, def.m_StartPosition.m_Max.z }
				}
			}*/
		};

		j["billboard_definitions"].push_back(definition);
	}

	j["materials"] = {};
	for (int i = 0; i < MAX_TRAIL_MATERIALS; i++) {
		auto &mat = TrailMaterials[i];
		if (mat.m_MaterialName.empty())
			continue;

		json material = {
			{"name", mat.m_MaterialName },
			{"path", mat.m_ShaderPath }
		};

		j["materials"].push_back(material);
	}

	j["textures"] = {};
	for (int i = 0; i < MAX_MATERIAL_TEXTURES; i++) {
		auto &tex = MaterialTextures[i];
		if (tex.m_TextureName.empty())
			continue;

		json material = {
			{ "name", tex.m_TextureName },
			{ "path", tex.m_TexturePath }
		};

		j["textures"].push_back(material);
	}

	j["fx"] = {};
	for (auto &fx : EffectDefinitions) {
		json entries = {};
		for (int i = 0; i < fx.m_Count; i++) {
			auto &pentry = fx.m_Entries[i];
			json entry;

			switch (pentry.type) {
				case ParticleType::Trail:
					entry["type"] = "trail";
					break;
				case ParticleType::Billboard:
					entry["type"] = "billboard";
					entry["name"] = pentry.billboard->name;
					break;
				case ParticleType::Geometry:
					entry["type"] = "geometry";
					break;
			}

			entries.push_back(entry);
		}

		json obj = {
			{"entries", entries},
			{"name", fx.name}
		};
		
		j["fx"].push_back(obj);
	}


	std::ofstream o("editor.json");
	o << std::setw(4) << j << std::endl;
}

void Run()
{
	/*TrailMaterials[0].m_MaterialName = "Default";
	TrailMaterials[0].m_ShaderPath = "Resources/Shaders/BillboardParticleSimple.hlsl";
	MaterialTextures[0].m_TextureName = "DefaultChecker";
	MaterialTextures[0].m_TexturePath = "Resources/Textures/Plane.dds";
	MaterialTextures[1].m_TextureName = "Noise";
	MaterialTextures[1].m_TexturePath = "Resources/Textures/Noise.png";

	BillboardDefinitions[0].name = "DefaultBillboard";
	BillboardDefinitions[0].lifetime = -1.f;
	BillboardDefinitions[0].m_Material = &TrailMaterials[0];

	ParticleEffect fx = {};
	fx.m_Count = 1;
	fx.m_Entries[0].type = ParticleType::Billboard;
	fx.m_Entries[0].billboard = &BillboardDefinitions[0];
	EffectDefinitions.push_back(fx);*/

	Load();
	Save();


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
	ImwWindow *particles = new ParticleList();
	ConsoleOutput = new Output();

	ConsoleOutput->AddLog("Particle Editor v0.1\n");
	ConsoleOutput->AddLog("LMB to rotate, RMB to drag\n");
	ConsoleOutput->AddLog("Ctrl-R to reload resources\n");

	manager.Dock(viewport, E_DOCK_ORIENTATION_CENTER);
	manager.Dock(textures, E_DOCK_ORIENTATION_RIGHT, 0.2f);
	manager.DockWith(materials, textures, E_DOCK_ORIENTATION_CENTER);
	manager.DockWith(attr, materials, E_DOCK_ORIENTATION_BOTTOM, 0.5f);

	manager.Dock(effects, E_DOCK_ORIENTATION_LEFT, 0.15);
	manager.DockWith(particles, effects, E_DOCK_ORIENTATION_BOTTOM, 0.5f);

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

		//ImGui::GetIO().DeltaTime = 0.016f;

		Sleep(1);
	} while (manager.Run(false) && manager.Run(true));
	
}

void Style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	auto accent = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);

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
