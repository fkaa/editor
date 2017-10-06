#pragma once

#include "Particle.h"
#include "Output.h"

#define MAX_TRAIL_MATERIALS 16
#define MAX_BILLBOARD_MATERIALS 16
#define MAX_MATERIAL_TEXTURES 16
#define MAX_BILLBOARD_PARTICLE_DEFINITIONS 32

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