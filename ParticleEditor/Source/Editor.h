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

namespace Editor {;

enum class AttributeType {
	Effect,
	Trail,
	Billboard,
	Geometry,
	Texture,
	Material
};

struct AttributeObject {
	AttributeType type;
	int index;
};

extern Output *ConsoleOutput;

extern bool UnsavedChanges;
extern AttributeObject SelectedObject;

extern MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
extern TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];
extern BillboardParticleDefinition BillboardDefinitions[MAX_BILLBOARD_PARTICLE_DEFINITIONS];

extern std::vector<ParticleEffect> EffectDefinitions;

extern ParticleEffect *SelectedEffect;

BillboardParticleDefinition *GetBillboardDef(std::string name);
TrailParticleMaterial *GetMaterial(std::string name);

void Reload(ID3D11Device *device);
void Run();

}