#pragma once

#include "Particle.h"
#include "Output.h"

#define MAX_TRAIL_MATERIALS 16
#define MAX_BILLBOARD_MATERIALS 16
#define MAX_MATERIAL_TEXTURES 16
#define MAX_BILLBOARD_PARTICLE_DEFINITIONS 32

#define GEOMETRY_ICON ICON_MD_LANGUAGE
#define BILLBOARD_ICON ICON_MD_CROP_FREE
#define TRAIL_ICON ICON_MD_GRAIN
#define FX_ICON ICON_MD_PHOTO_FILTER


static const ImVec4 FX_COLORS[3] = {
	ImColor(0xFFBA68C8),
	ImColor(0xFFAB47BC),
	ImColor(0xFF9C27B0)
};

static const ImVec4 BILLBOARD_COLORS[3] = {
	ImColor(0xFF009688),
	ImColor(0xFF00897B),
	ImColor(0xFF00796B)
};

static const ImVec4 GEOMETRY_COLORS[3] = {
	ImColor(0xE91E63FF),
	ImColor(0xD81B60FF),
	ImColor(0xC2185BFF)
};

static const ImVec4 BUTTON_COLORS[3] = {
	ImColor(120, 144, 156, 0),
	ImColor(96, 125, 139),
	ImColor(84, 110, 122)
};

static const ImVec4 SELECT_COLORS[3] = {
	ImColor(117, 117, 117),
	ImColor(97, 97, 97),
	ImColor(66, 66, 66)
};


struct MaterialTexture {
	std::string m_TextureName;
	std::string m_TexturePath;
	ID3D11ShaderResourceView *m_SRV;
};

inline void ComboFunc(const char *label, ParticleEase *ease)
{
	ImGui::Combo(label, (int*)ease, EASE_STRINGS);
}

namespace Editor {;

enum class AttributeType {
	None,
	Effect,
	Trail,
	Billboard,
	GeometryEntry,
	Geometry,
	Texture,
	Material
};

struct AttributeObject {
	AttributeType type;
	int index;
};

extern Output *ConsoleOutput;

extern float Speed;

extern bool UnsavedChanges;
extern AttributeObject SelectedObject;

extern MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
extern TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];
extern BillboardParticleDefinition BillboardDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];
extern GeometryParticleDefinition GeometryDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];
extern TrailParticleDefinition TrailDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];

extern std::vector<ParticleEffect> EffectDefinitions;

extern ParticleEffect *SelectedEffect;

BillboardParticleDefinition *GetBillboardDef(std::string name);
GeometryParticleDefinition *GetGeometryDef(std::string name);
TrailParticleDefinition *GetTrailDef(std::string name);

TrailParticleMaterial *GetMaterial(std::string name);

void Reload(ID3D11Device *device);
void Run();

}