#pragma once

#include "Particle.h"
#include "Output.h"

#define MAX_TRAIL_MATERIALS 16
#define MAX_MATERIAL_TEXTURES 16

struct MaterialTexture {
	std::string m_TextureName;
	std::string m_TexturePath;
	ID3D11ShaderResourceView *m_SRV;
};

namespace Editor {;

extern Output *ConsoleOutput;
extern bool UnsavedChanges;

extern MaterialTexture MaterialTextures[MAX_MATERIAL_TEXTURES];
extern TrailParticleMaterial TrailMaterials[MAX_TRAIL_MATERIALS];

void Reload(ID3D11Device *device);
void Run();

}