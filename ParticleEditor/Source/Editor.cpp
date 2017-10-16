#include "Editor.h"

#include <Windows.h>

#include <vector>
#include <algorithm>

#include <cstdio>

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

static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}



using json = nlohmann::json;

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
			if (ImGui::MenuItem("Open", "CTRL+O", nullptr, false))
			{

			}

			if (ImGui::MenuItem("Save", "CTRL+O", nullptr, false))
			{

			}
			if (ImGui::MenuItem("Save As", "CTRL+O", nullptr, false))
			{

			}

			if (ImGui::MenuItem("Export", "CTRL+E", nullptr, true))
			{
				OPENFILENAMEA ofn;
				char szFileName[MAX_PATH] = "";

				ZeroMemory(&ofn, sizeof(ofn));

				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = ImwPlatformWindowDX11::s_mInstances.begin()->first;
				ofn.lpstrFilter = "part Files (*.part)\0*.part\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt = "part";

				if (GetSaveFileNameA(&ofn)) {
					Editor::Export(ofn.lpstrFile);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "ALT-F4"))
			{
				ImWindow::ImwWindowManager::GetInstance()->Destroy();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings")) {
			ImGui::DragFloat("speed##settings", &Editor::Speed, 0.01f, 0.f, 5.f, "%.1fx speed");

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
		
		ImGui::PushStyleColor(ImGuiCol_Button,        FX_COLORS[0]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, FX_COLORS[1]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  FX_COLORS[2]);
		if (ImGui::Button(FX_ICON " [FX]")) {
			ParticleEffect effect = {};
			snprintf(effect.name, 16, "FX#%d", Editor::EffectDefinitions.size());
			Editor::EffectDefinitions.push_back(effect);
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Particle FX");
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::TextDisabled("|");

		ImGui::SameLine();


				
		//ImGui::PushStyleColor(ImGuiCol_Button,        TRAIL_COLORS[0]);
		//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TRAIL_COLORS[1]);
		//ImGui::PushStyleColor(ImGuiCol_ButtonActive,  TRAIL_COLORS[2]);
		ImGui::Button(TRAIL_ICON " [Trail]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Particle Trail");
		//ImGui::PopStyleColor(3);

		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button,        BILLBOARD_COLORS[0]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BILLBOARD_COLORS[1]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  BILLBOARD_COLORS[2]);
		ImGui::Button(BILLBOARD_ICON " [Billboard]");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Billboard Particle");
		ImGui::PopStyleColor(3);

		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button,        GEOMETRY_COLORS[0]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GEOMETRY_COLORS[1]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  GEOMETRY_COLORS[2]);
		if (ImGui::Button(GEOMETRY_ICON " [Sphere]")) {

		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Create new Sphere Particle");
		ImGui::PopStyleColor(3);

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
				auto w = ImGui::GetContentRegionAvailWidth();
				if (tex.m_SRV)
					ImGui::Image(tex.m_SRV, ImVec2(w, w));

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
			case Editor::AttributeType::GeometryEntry: {
				auto &entry = Editor::SelectedEffect->m_Entries[Editor::SelectedObject.index];
				ImGui::TextColored(FX_COLORS[0], FX_ICON " %s[%d] > " GEOMETRY_ICON " %s", Editor::SelectedEffect->name, Editor::SelectedObject.index, entry.geometry->name.c_str());
				ImGui::Separator();

				ImGui::Text("Start Position");
				ImGui::DragFloat3("min##start", (float*)&entry.m_StartPosition.m_Min, 0.005f);
				ImGui::DragFloat3("max##start", (float*)&entry.m_StartPosition.m_Max, 0.005f);

				ImGui::Text("Start Velocity");
				ImGui::DragFloat3("min##vel", (float*)&entry.m_StartVelocity.m_Min, 0.005f);
				ImGui::DragFloat3("max##vel", (float*)&entry.m_StartVelocity.m_Max, 0.005f);

				ImGui::Text("Rotation Axis");
				ImGui::DragFloat("min##rotaxis", &entry.m_RotLimitMin, 0.005f);
				ImGui::DragFloat("max##rotaxis", &entry.m_RotLimitMax, 0.005f);


				ImGui::Text("Rotation Speed");
				ImGui::DragFloat("min##rotspeed", &entry.m_RotSpeedMin, 0.005f);
				ImGui::DragFloat("max##rotspeed", &entry.m_RotSpeedMax, 0.005f);


				ImGui::Text("Spawn Rate");
				ComboFunc("easing##spawn", &entry.m_SpawnEasing);
				ImGui::DragFloat("start##spawn", (float*)&entry.m_SpawnStart, 0.005f);
				ImGui::DragFloat("end##spawn", (float*)&entry.m_SpawnEnd, 0.005f);

				ImGui::Separator();

				auto &def = *entry.geometry;
				ImGui::TextColored(GEOMETRY_COLORS[0], GEOMETRY_ICON " %s (" ICON_MD_LINK ")", entry.geometry->name.c_str());
				ImGui::SameLine();
				ShowHelpMarker("This will edit the referenced definition as well");
				ImGui::Separator();

				ImGui::DragFloat("lifetime", &def.lifetime, 0.005f);
				ImGui::DragFloat("gravity", &def.m_Gravity, 0.005f);

				ImGui::Text("Noise");
				ImGui::DragFloat("scale##noise", &def.m_NoiseScale, 0.005f);
				ImGui::DragFloat("speed##noise", &def.m_NoiseSpeed, 0.005f);

				ImGui::Text("Deform");
				ImGui::DragFloat("speed##Deform", &def.m_DeformSpeed, 0.005f);
				ComboFunc("easing##Deform", &def.m_DeformEasing);
				ImGui::DragFloat("start##Deform", &def.m_DeformFactorStart, 0.005f);
				ImGui::DragFloat("end##Deform", &def.m_DeformFactorEnd, 0.005f);

				ImGui::Text("Size");  
				ComboFunc("easing##Size",       &def.m_SizeEasing);
				ImGui::DragFloat("start##Size", &def.m_SizeStart, 0.005f);
				ImGui::DragFloat("end##Size",   &def.m_SizeEnd, 0.005f);

				ImGui::Text("Color");
				ComboFunc("easing##Color",        &def.m_ColorEasing);
				ImGui::ColorEdit4("start##Color", (float*)&def.m_ColorStart);
				ImGui::ColorEdit4("end##Color",   (float*)&def.m_ColorEnd);

			} break;
			case Editor::AttributeType::Geometry: {
				auto &def = Editor::GeometryDefinitions[Editor::SelectedObject.index];
				ImGui::TextColored(GEOMETRY_COLORS[0], GEOMETRY_ICON " %s", def.name.c_str());
				ImGui::Separator();

				ImGui::DragFloat("lifetime", &def.lifetime);
				ImGui::DragFloat("gravity", &def.m_Gravity);

				ImGui::Text("Noise");
				ImGui::DragFloat("scale##noise", &def.m_NoiseScale, 0.005f);
				ImGui::DragFloat("speed##noise", &def.m_NoiseSpeed, 0.005f);

				ImGui::Text("Deform");
				ImGui::DragFloat("speed##Deform", &def.m_DeformSpeed, 0.005f);
				ComboFunc("easing##Deform", &def.m_DeformEasing);
				ImGui::DragFloat("start##Deform", &def.m_DeformFactorStart, 0.005f);
				ImGui::DragFloat("end##Deform", &def.m_DeformFactorEnd, 0.005f);

				ImGui::Text("Size");
				ComboFunc("easing##Size", &def.m_SizeEasing);
				ImGui::DragFloat("start##Size", &def.m_SizeStart, 0.005f);
				ImGui::DragFloat("end##Size", &def.m_SizeEnd, 0.005f);

				ImGui::Text("Color");
				ComboFunc("easing##Color", &def.m_ColorEasing);
				ImGui::ColorEdit4("start##Color", (float*)&def.m_ColorStart);
				ImGui::ColorEdit4("end##Color", (float*)&def.m_ColorEnd);
			} break;
			case Editor::AttributeType::None:
				ImGui::Text("No selection");
				break;
			default:
				ImGui::Text("Not implemented :-(");
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
		static int billboard_selected = 0;
		static int geom_selected = 0;
		static int tab = 2;
		ImGui::RadioButton(BILLBOARD_ICON "##billboard", &tab, (int)ParticleType::Billboard);
		ImGui::SameLine();
		ImGui::RadioButton(TRAIL_ICON "##trail", &tab, (int)ParticleType::Trail);
		ImGui::SameLine();
		ImGui::RadioButton(GEOMETRY_ICON "##geometry", &tab, (int)ParticleType::Geometry);
		
		ImGui::BeginChild("ParticleList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
		switch ((ParticleType)tab) {
			case ParticleType::Billboard: {
				for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++)
				{
					auto &def = Editor::BillboardDefinitions[i];
					if (def.name.empty())
						continue;

					char label[128];
					sprintf(label, BILLBOARD_ICON " %s", def.name.c_str());
					if (ImGui::Selectable(label, billboard_selected == i)) {
						Editor::SelectedObject = {
							Editor::AttributeType::Billboard,
							i
						};
					}
				}
			} break;
			case ParticleType::Geometry: {
				for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++)
				{
					auto &def = Editor::GeometryDefinitions[i];
					if (def.name.empty())
						continue;

					char label[128];
					sprintf(label, GEOMETRY_ICON " %s", def.name.c_str());
					if (ImGui::Selectable(label, geom_selected == i)) {
						Editor::SelectedObject = {
							Editor::AttributeType::Geometry,
							i
						};
					}
				}
			} break;
			default:
				break;
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
			sprintf(label, FX_ICON " %s (%d)", fx.name, fx.m_Count);
			bool node = ImGui::TreeNode(label);
			
			ImGui::SameLine();
			
			ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 42);
			sprintf(label, ICON_MD_PLAY_ARROW "##%s%d", fx.name, i);
			if (ImGui::Button(label)) {
				Editor::SelectedEffect = &Editor::EffectDefinitions[i];
			}
			
			ImGui::SameLine();

			ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 14);
			sprintf(label, ICON_MD_ZOOM_IN "##%s%d", fx.name, i);
			if (ImGui::Button(label)) {
				Editor::SelectedObject = Editor::AttributeObject{
					Editor::AttributeType::Effect,
					i
				};
			}

			if (node) {
				for (int j = 0; j < fx.m_Count; j++) {
					auto &entry = fx.m_Entries[j];

					switch (entry.type) {
						case ParticleType::Billboard: {
							auto def = entry.billboard;
							sprintf(label, "[%d] " BILLBOARD_ICON " %s", j, def->name.c_str());
							if (ImGui::Selectable(label, selected == j)) {
								Editor::SelectedEffect = &Editor::EffectDefinitions[i];
								Editor::SelectedObject = {
									Editor::AttributeType::Billboard,
									(int)(def - Editor::BillboardDefinitions)
								};
							}
						} break;
						case ParticleType::Geometry: {
							auto def = entry.geometry;
							sprintf(label, "[%d] " GEOMETRY_ICON " %s##%s%d", j, def->name.c_str(), fx.name, j);
							if (ImGui::Selectable(label, selected == j)) {
								Editor::SelectedEffect = &Editor::EffectDefinitions[i];
								Editor::SelectedObject = {
									Editor::AttributeType::GeometryEntry,
									j
								};
							}
						} break;
						case ParticleType::Trail: {
							auto def = entry.trail.def;
							sprintf(label, "[%d] " TRAIL_ICON " %s", j, def->name.c_str());
							if (ImGui::Selectable(label, selected == j)) {
								Editor::SelectedEffect = &Editor::EffectDefinitions[i];
								Editor::SelectedObject = {
									Editor::AttributeType::Trail,
									(int)(def - Editor::TrailDefinitions)
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

float Speed = 1.f;

bool Debug = false;
bool UnsavedChanges = true;
AttributeObject SelectedObject;

MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];
BillboardParticleDefinition BillboardDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];
GeometryParticleDefinition GeometryDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];
TrailParticleDefinition TrailDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];

std::vector<ParticleEffect> EffectDefinitions;

ParticleEffect *SelectedEffect;

JsonValue EditorState;

PosBox GetPositionBox(json &table, std::string name)
{
	auto box = table[name];
	auto min = box["min"];
	auto max = box["max"];

	PosBox vel = {
		{ min[0], min[1], min[2] },
		{ max[0], max[1], max[2] },
	};

	return vel;
}


VelocityBox GetVelocityBox(json &table, std::string name)
{
	auto box = table[name];
	auto min = box["min"];
	auto max = box["max"];

	VelocityBox vel = {
		{ min[0], min[1], min[2] },
		{ max[0], max[1], max[2] },
	};

	return vel;
}

SimpleMath::Vector4 GetVector4(json &vec)
{
	return { vec[0], vec[1], vec[2], vec[3] };
}

BillboardParticleDefinition *GetBillboardDef(std::string name)
{
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto &def = BillboardDefinitions[i];
		if (def.name == name)
			return &BillboardDefinitions[i];
	}

	return nullptr;
}

GeometryParticleDefinition *GetGeometryDef(std::string name)
{
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto &def = GeometryDefinitions[i];
		if (def.name == name)
			return &GeometryDefinitions[i];
	}

	return nullptr;
}

TrailParticleDefinition * GetTrailDef(std::string name)
{
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto &def = TrailDefinitions[i];
		if (def.name == name)
			return &TrailDefinitions[i];
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

	auto gdefs = data.at("geometry_definitions");
	auto gd = GeometryDefinitions;
	for (auto entry : gdefs) {
		GeometryParticleDefinition def = {};
		std::string n = entry["name"];
		def.name = n;
		def.m_Material = GetMaterial(entry["material_name"]);
		def.lifetime = entry["lifetime"];
		def.m_Gravity = entry["gravity"];
		def.m_NoiseScale = entry["noise_scale"];
		def.m_NoiseSpeed = entry["noise_speed"];

		auto color = entry["color"];
		def.m_ColorEasing = GetEasingFromString(color["function"]);
		def.m_ColorStart = GetVector4(color["start"]);
		def.m_ColorEnd = GetVector4(color["end"]);

		auto deform = entry["deform"];
		def.m_DeformEasing = GetEasingFromString(deform["function"]);
		def.m_DeformFactorStart = deform["start"];
		def.m_DeformFactorEnd = deform["end"];

		def.m_DeformSpeed = entry["deform_speed"];
		
		auto size = entry["size"];
		def.m_SizeEasing = GetEasingFromString(size["function"]);
		def.m_SizeStart = size["start"];
		def.m_SizeEnd = size["end"];

		*gd++ = def;
	}


	auto tdefs = data.at("trail_definitions");
	auto td = TrailDefinitions;
	for (auto entry :tdefs) {
		TrailParticleDefinition def = {};
		std::string n = entry["name"];
		def.name = n;
		def.lifetime = entry["lifetime"];
		def.frequency = entry["frequency"];
		def.m_Gravity = entry["gravity"];
		def.m_Material = GetMaterial(entry["material_name"]);
		//def.m_StartPosition = GetPositionBox(entry, "start_position");
		//def.m_StartVelocity = GetVelocityBox(entry, "start_velocity");
		//def.lifetime = entry["lifetime"];
		*td++ = def;
	}

	auto effects = data.at("fx");
	for (auto entry : effects) {
		
		ParticleEffect fx = {};
		std::string name = entry["name"];
		memcpy(fx.name, name.data(), min(name.size(), 15));
		for (int i = name.size(); i < 15; i++)
			fx.name[i] = '\0';

		auto entries = entry["entries"];
		for (auto fxentry : entries) {
			auto type = ParticleTypeFromString(fxentry["type"]);
			std::string name = fxentry["name"];
			
			ParticleEffectEntry ent = {};
			ent.type = type;

			ent.start = fxentry["start"];
			ent.time = fxentry["time"];

			ent.m_StartPosition = GetPositionBox(fxentry, "start_position");
			ent.m_StartVelocity = GetVelocityBox(fxentry, "start_velocity");

			auto spawn = fxentry["spawn"];
			ent.m_SpawnEasing  = GetEasingFromString(spawn["function"]);
			ent.m_SpawnStart = spawn["start"];
			ent.m_SpawnEnd = spawn["end"];

			auto rotlim = fxentry["rotation"];
			ent.m_RotLimitMin = rotlim["min"];
			ent.m_RotLimitMax = rotlim["max"];

			auto rot = fxentry["rotation_speed"];
			ent.m_RotSpeedMin = rot["min"];
			ent.m_RotSpeedMax = rot["max"];

			switch (type) {
				case ParticleType::Billboard:
					ent.billboard = GetBillboardDef(name);
					break;
				case ParticleType::Geometry:
					ent.geometry = GetGeometryDef(name);
					break;
				case ParticleType::Trail:
					ent.trail = {
						GetTrailDef(name),
						-1
					};
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
			{ "name", def.name },
			{ "material_name", def.m_Material->m_MaterialName },
			{ "lifetime", def.lifetime },
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

	j["geometry_definitions"] = {};
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto def = GeometryDefinitions[i];
		if (def.name.empty())
			continue;

		json definition = {
			{ "name", def.name },
			{ "material_name", def.m_Material->m_MaterialName },
			{ "lifetime", def.lifetime },
			{ "gravity", def.m_Gravity },
			{ "noise_scale", def.m_NoiseScale },
			{ "noise_speed", def.m_NoiseSpeed },
			{ "color", {
				{ "start", { def.m_ColorStart.x, def.m_ColorStart.y, def.m_ColorStart.z, def.m_ColorStart.w } },
				{ "end", { def.m_ColorEnd.x, def.m_ColorEnd.y, def.m_ColorEnd.z, def.m_ColorEnd.w } },
				{ "function", GetEasingName(def.m_ColorEasing) }
			}},
			{ "deform", {
				{ "start", def.m_DeformFactorStart },
				{ "end", def.m_DeformFactorEnd },
				{ "function", GetEasingName(def.m_DeformEasing) }
			}},
			{ "deform_speed", def.m_DeformSpeed },
			{ "size", {
				{ "start", def.m_SizeStart },
				{ "end", def.m_SizeEnd },
				{ "function", GetEasingName(def.m_SizeEasing) }
			}}
		};

		j["geometry_definitions"].push_back(definition);
	}

	j["trail_definitions"] = {};
	for (int i = 0; i < MAX_BILLBOARD_PARTICLE_DEFINITIONS; i++) {
		auto def = TrailDefinitions[i];
		if (def.name.empty())
			continue;

		json definition = {
			{ "name", def.name },
			{ "material_name", def.m_Material->m_MaterialName },
			{ "lifetime", def.lifetime },
			{ "gravity", def.m_Gravity },
			{ "frequency", def.frequency }
		};

		j["trail_definitions"].push_back(definition);
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
					entry["name"] = pentry.trail.def->name;
					break;
				case ParticleType::Billboard:
					entry["type"] = "billboard";
					entry["name"] = pentry.billboard->name;
					break;
				case ParticleType::Geometry: {
					entry["type"] = "geometry";
					entry["name"] = pentry.geometry->name;
				} break;
			}

			entry["start"] = pentry.start;
			entry["time"] = pentry.time;
			entry["spawn"] = {
				{ "start", pentry.m_SpawnStart },
				{ "end", pentry.m_SpawnEnd },
				{ "function", GetEasingName(pentry.m_SpawnEasing) }
			};
			entry["start_position"] = {
				{ "min",{ pentry.m_StartPosition.m_Min.x, pentry.m_StartPosition.m_Min.y, pentry.m_StartPosition.m_Min.z } },
				{ "max",{ pentry.m_StartPosition.m_Max.x, pentry.m_StartPosition.m_Max.y, pentry.m_StartPosition.m_Max.z } }
			};
			entry["start_velocity"] = {
				{ "min",{ pentry.m_StartVelocity.m_Min.x, pentry.m_StartVelocity.m_Min.y, pentry.m_StartVelocity.m_Min.z } },
				{ "max",{ pentry.m_StartVelocity.m_Max.x, pentry.m_StartVelocity.m_Max.y, pentry.m_StartVelocity.m_Max.z } }
			};
			entry["rotation"] = {
				{ "min", pentry.m_RotLimitMin },
				{ "max", pentry.m_RotLimitMax }
			};
			entry["rotation_speed"] = {
				{ "min", pentry.m_RotSpeedMin },
				{ "max", pentry.m_RotSpeedMax }
			};

			entries.push_back(entry);
		}

		json obj = {
			{"entries", entries},
			{"name", fx.name},
			{"time", fx.time}
		};
		
		j["fx"].push_back(obj);
	}


	std::ofstream o("editor.json");
	o << std::setw(4) << j << std::endl;
}

/*
 * File format:
 *   magic: [char; 4],
 *   texture_count: u32,
 *   texture_paths: [[char; 128]; texture_count],
 *   material_count: u32,
 *   material_paths: [[char; 128]; material_count],
 *   geometry_count: u32,
 *   geometry: [GeometryDefinition; geometry_count],
 *   fx_count: u32,
 *   fx: [ParticleEffect; fx_count]
 *
 * Types:
 *   ParticleType: enum {
 *     Trail = 0,
 *     Billboard = 1,
 *     Geometry = 2,
 *   }
 *
 *   ParticleEase: enum {
 *     Linear = 0,
 *     EaseIn = 1,
 *     EaseOut = 2,
 *   }
 *
 *   GeometryDefinition: struct {
 *     mat_idx: i32,
 *     lifetime: f32,
 *     gravity: f32,
 *     noise_scale: f32,
 *     noise_speed: f32,
 *     deform_easing: ParticleEase,
 *     deform_start: f32,
 *     deform_end: f32,
 *     deform_speed: f32,
 *     size_easing: ParticleEase,
 *     size_start: f32,
 *     size_end: f32,
 *     color_easing: ParticleEase,
 *     color_start: [f32; 4],
 *     color_end: [f32; 4],
 *   }
 *
 *   ParticleEffectEntry: struct {
 *     type: ParticleType,
 *     def_idx: i32,
 *     start: f32,
 *     time: f32,
 *     loop: bool,
 *     start_position: [[f32; 3]; 2],
 *     start_velocity: [[f32; 3]; 2],
 *     spawn_easing: ParticleEase,
 *     spawn_start: f32,
 *     spawn_end: f32,
 *     rot_limit_min: f32,
 *     rot_limit_max: f32,
 *     rot_speed_min: f32,
 *     rot_speed_max: f32,
 *   }
 *
 *   ParticleEffect: {
 *     name: [char; 16],
 *     count: u32,
 *     time: f32,
 *     entry_count: u32,
 *     entries: [ParticleEffectEntry; entry_count],
 *   }
 */
void Export(const char *file)
{
	FILE *f = fopen(file, "w+");
	if (!f) {
		char err[128];
		strerror_s(err, errno);
		ConsoleOutput->AddLog("Failed to export particle data to %s (%s)\n", file, err);
	}

	// magic 4-byte constant
	char magic[4] = { 'p', 'a', 'r', 't' };
	fwrite(magic, sizeof(magic), 1, f);

	uint32_t texture_count = 0;

	std::vector<std::array<char, 128>> paths;
	for (; texture_count < MAX_MATERIAL_TEXTURES; texture_count++) {
		auto &tex = MaterialTextures[texture_count];
		if (tex.m_TextureName.empty())
			break;

		std::array<char, 128> path = { '\0' };
		strcpy_s(path.data(), 128, tex.m_TextureName.c_str());

		paths.push_back(path);
	}

	fwrite(&texture_count, sizeof(uint32_t), 1, f);
	fwrite(paths.data(), sizeof(char[128]), texture_count, f);

	paths.clear();

	uint32_t material_count = 0;
	for (; material_count < MAX_TRAIL_MATERIALS; material_count++) {
		auto &mat = TrailMaterials[texture_count];
		if (mat.m_MaterialName.empty())
			break;

		std::array<char, 128> path = {'\0'};
		strcpy_s(path.data(), 127, mat.m_MaterialName.c_str());

		paths.push_back(path);
	}

	fwrite(&material_count, sizeof(uint32_t), 1, f);
	fwrite(paths.data(), sizeof(char[128]), material_count, f);

	uint32_t geometry_count = 0;
	for (; geometry_count < MAX_BILLBOARD_PARTICLE_DEFINITIONS; geometry_count++) {
		auto &def = GeometryDefinitions[geometry_count];
		if (def.name.empty())
			break;
	}

	fwrite(&geometry_count, sizeof(uint32_t), 1, f);

	for (int i = 0; i < geometry_count; i++) {
		auto &def = GeometryDefinitions[i];
		
		int32_t mat_idx = (def.m_Material - Editor::TrailMaterials);
		fwrite(&mat_idx, sizeof(int32_t), 1, f);
		fwrite(&def.lifetime, sizeof(float), 1, f);
		fwrite(&def.m_Gravity, sizeof(float), 1, f);
		fwrite(&def.m_NoiseScale, sizeof(float), 1, f);
		fwrite(&def.m_NoiseSpeed, sizeof(float), 1, f);
		fwrite(&def.m_DeformEasing, sizeof(ParticleEase), 1, f);
		fwrite(&def.m_DeformFactorStart, sizeof(float), 1, f);
		fwrite(&def.m_DeformFactorEnd, sizeof(float), 1, f);
		fwrite(&def.m_DeformSpeed, sizeof(float), 1, f);
		fwrite(&def.m_SizeEasing, sizeof(ParticleEase), 1, f);
		fwrite(&def.m_SizeStart, sizeof(float), 1, f);
		fwrite(&def.m_SizeEnd, sizeof(float), 1, f);
		fwrite(&def.m_ColorEasing, sizeof(ParticleEase), 1, f);
		fwrite(&def.m_ColorStart, sizeof(SimpleMath::Vector4), 1, f);
		fwrite(&def.m_ColorEnd, sizeof(SimpleMath::Vector4), 1, f);
	}

	uint32_t fx_count = EffectDefinitions.size();
	fwrite(&fx_count, sizeof(uint32_t), 1, f);

	for (int j = 0; j < fx_count - 1; j++) {
		auto &fx = EffectDefinitions[j];

		fwrite(&fx.name, sizeof(fx.name), 1, f);
		fwrite(&fx.m_Count, sizeof(uint32_t), 1, f);
		fwrite(&fx.time, sizeof(float), 1, f);

		uint32_t entry_count = fx.m_Count;

		for (int i = 0; i < entry_count; i++) {
			auto &entry = fx.m_Entries[i];

			fwrite(&entry.type, sizeof(ParticleType), 1, f);
			switch (entry.type) {
				case ParticleType::Geometry: {
					int32_t def_idx = entry.geometry - GeometryDefinitions;
					fwrite(&def_idx, sizeof(int32_t), 1, f);
				} break;
				default:
					break;
			}

			fwrite(&entry.start, sizeof(float), 1, f);
			fwrite(&entry.time, sizeof(float), 1, f);
			fwrite(&entry.m_Loop, sizeof(bool), 1, f);
			fwrite(&entry.m_StartPosition.m_Min, sizeof(SimpleMath::Vector3), 1, f);
			fwrite(&entry.m_StartPosition.m_Max, sizeof(SimpleMath::Vector3), 1, f);
			fwrite(&entry.m_StartVelocity.m_Min, sizeof(SimpleMath::Vector3), 1, f);
			fwrite(&entry.m_StartVelocity.m_Max, sizeof(SimpleMath::Vector3), 1, f);
			fwrite(&entry.m_SpawnEasing, sizeof(ParticleEase), 1, f);
			fwrite(&entry.m_SpawnStart, sizeof(float), 1, f);
			fwrite(&entry.m_SpawnEnd, sizeof(float), 1, f);
			fwrite(&entry.m_RotLimitMin, sizeof(float), 1, f);
			fwrite(&entry.m_RotLimitMax, sizeof(float), 1, f);
			fwrite(&entry.m_RotSpeedMin, sizeof(float), 1, f);
			fwrite(&entry.m_RotSpeedMax, sizeof(float), 1, f);
		}
	}

	fclose(f);

	ConsoleOutput->AddLog("Exported particle data to %s\n", file);
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

	EffectDefinitions.reserve(128);

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
	ConsoleOutput->AddLog("LMB to rotate, RMB to drag, MMB to zoom\n");
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
	style.Colors[ImGuiCol_Border] = SELECT_COLORS[2];
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] =        SELECT_COLORS[2];//ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = SELECT_COLORS[1];//ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] =  SELECT_COLORS[0];//ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 0.725f, 0.0f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = BUTTON_COLORS[1];
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.95f, 0.92f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.92f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_Button] = BUTTON_COLORS[0];
	style.Colors[ImGuiCol_ButtonHovered] = BUTTON_COLORS[1];
	style.Colors[ImGuiCol_ButtonActive] = BUTTON_COLORS[2];
	style.Colors[ImGuiCol_Header] = SELECT_COLORS[2];//ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = SELECT_COLORS[1];//ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = SELECT_COLORS[0];// ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
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
