#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include "Ease.h"
#include <d3d11.h>
#include <SimpleMath.h>


using namespace DirectX;






using ParticleShaderID = uint64_t;

struct VelocityBox {
	SimpleMath::Vector3 m_Min;
	SimpleMath::Vector3 m_Max;
};

struct PosBox {
	SimpleMath::Vector3 m_Min;
	SimpleMath::Vector3 m_Max;
};

struct TrailParticleMaterial {
	std::string m_MaterialName;
	std::string m_ShaderPath;
	ID3D11PixelShader *m_PixelShader;
};

struct TrailParticleDefinition {
	TrailParticleMaterial m_Material;
	PosBox m_StartPosition;
	VelocityBox m_StartVelocity;
	VelocityBox m_Gravity;
	VelocityBox m_TurbulenceStart;
	VelocityBox m_TurbulenceEnd;
	SimpleMath::Vector2 m_SizeStart;
	SimpleMath::Vector2 m_SizeEnd;
	float frequency;
};

struct TrailParticleEffect {
	TrailParticleDefinition *m_Definition;

};

struct TrailParticle {
	SimpleMath::Vector3 m_Position;
	SimpleMath::Vector2 m_Size;
};


using Trail = TrailParticle[16];



enum class TrailParticleEnvironment {
	Circle,
	Sphere,
	Line
};


struct GeometryParticleDefinition {
	std::string name;
	TrailParticleMaterial *m_Material;
	PosBox m_StartPosition;
	VelocityBox m_StartVelocity;
	VelocityBox m_Gravity;
	VelocityBox m_TurbulenceStart;
	VelocityBox m_TurbuldsenceEnd;
	SimpleMath::Vector2 m_SizeStart;
	SimpleMath::Vector2 m_SizeEnd;
	float lifetime;

	int m_ModelID;
};



struct BillboardParticleDefinition {
	std::string name;
	TrailParticleMaterial *m_Material;
	PosBox m_StartPosition;
	VelocityBox m_StartVelocity;
	VelocityBox m_Gravity;
	VelocityBox m_TurbulenceStart;
	VelocityBox m_TurbuldsenceEnd;
	SimpleMath::Vector2 m_SizeStart;
	SimpleMath::Vector2 m_SizeEnd;
	float lifetime;
};

enum class ParticleType {
	Trail,
	Billboard,
	Geometry
};

inline ParticleType ParticleTypeFromString(std::string str)
{
	if (str == "trail") return ParticleType::Trail;
	if (str == "billboard") return ParticleType::Billboard;
	if (str == "geometry") return ParticleType::Geometry;
	
	assert(false);
}

struct ParticleEffectEntry {
	ParticleType type;
	union {
		GeometryParticleDefinition *geometry;
		BillboardParticleDefinition *billboard;
		TrailParticleDefinition *trail;
	};
};

struct ParticleEffect {
	char name[16] = "Untitled\0";
	int m_Count;
	float age;
	ParticleEffectEntry m_Entries[8];
};

struct BillboardParticle {
	XMFLOAT3 position;
	XMFLOAT2 size;
	float age;
	int idx;
};

struct GeometryParticle {
	XMMATRIX model;
	float age;
	int idx;
};



namespace old {
#define EMITTER_STRINGS "Static\0Box\0Sphere\0"

	enum class ParticleEmitter {
		Static = 0,
		Cube,
		Sphere
	};

	enum class ParticleOrientation {
		Planar = 0,
		Clip,
		Velocity,
		VelocityAnchored
	};

	enum class ParticleEase {
		Linear = 0,
		EaseIn,
		EaseOut,
		None
	};

	typedef float(*EaseFunc)(float, float, float);
	typedef XMVECTOR(*EaseFuncV)(XMVECTOR, XMVECTOR, float);

	extern EaseFunc ease_funcs[4];
	extern EaseFuncV ease_funcs_xmv[4];

#define EASE_STRINGS "Linear\0EaseIn\0EaseOut\0"
#define EASE_STRINGS_OPTIONAL "Linear\0EaseIn\0EaseOut\0None\0"

	inline EaseFunc GetEaseFunc(ParticleEase ease)
	{
		return ease_funcs[(int)ease];
	}

	inline EaseFuncV GetEaseFuncV(ParticleEase ease)
	{
		return ease_funcs_xmv[(int)ease];
	}


#define MAX_PARTICLE_FX 16

	struct ParticleDefinition {
		char name[32] = { "Untitled\0" };
		ParticleOrientation orientation;

		float gravity;
		float lifetime = 1.f;

		ParticleEase scale_fn;
		float scale_start;
		float scale_end = 1.0;

		ParticleEase distort_fn = ParticleEase::None;
		float distort_start = 1.0;
		float distort_end;

		ParticleEase color_fn;
		XMFLOAT4 start_color = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4 end_color = { 1.f, 1.f, 1.f, 0.f };

		int u, v, u2 = 256, v2 = 256;
	};

	struct ParticleEffectEntry {
		int idx = -1;
		float start, end = 1.f;
		bool loop;

		ParticleEmitter emitter_type;
		float emitter_xmin, emitter_xmax;
		float emitter_ymin, emitter_ymax;
		float emitter_zmin, emitter_zmax;

		float vel_xmin, vel_xmax;
		float vel_ymin, vel_ymax;
		float vel_zmin, vel_zmax;

		float rot_min, rot_max;
		float rot_vmin, rot_vmax;

		ParticleEase spawn_fn;
		int spawn_start = 0, spawn_end;
		float spawned_particles = 1.f;
	};

	struct ParticleEffect {
		char name[32] = { "Untitled\0" };
		int fx_count = 0;
		ParticleEffectEntry fx[MAX_PARTICLE_FX];
		float children_time = 0.f;
		float time = 1.f;
		float age;

		bool loop;
		bool clamp_children = false;
	};

	struct ParticleInstance {
		XMVECTOR origin;
		XMVECTOR pos;
		XMVECTOR velocity;
		XMVECTOR color;
		XMFLOAT4 uv;
		XMFLOAT2 scale;

		float rotation;
		float rotation_velocity;
		float age;
		float distort;
		int type;
		int idx;
	};


	inline bool SerializeParticles(const wchar_t *file, std::vector<ParticleEffect> effects, std::vector<ParticleDefinition> definitions)
	{
		FILE *f;

		if (_wfopen_s(&f, file, L"wb+") != 0) {
			return false;
		}

		for (auto &fx : effects) {
			fx.age = 0;
			for (int i = 0; i < fx.fx_count; ++i) {
				if (fx.fx[i].emitter_type == ParticleEmitter::Static)
					fx.fx[i].spawned_particles = 1.f;
			}
		}

		auto size = definitions.size();
		fwrite(&size, sizeof(size_t), 1, f);
		fwrite(definitions.data(), sizeof(ParticleDefinition), definitions.size(), f);

		size = effects.size();
		fwrite(&size, sizeof(size_t), 1, f);
		fwrite(effects.data(), sizeof(ParticleEffect), effects.size(), f);

		fclose(f);

		return true;
	}

	inline bool DeserializeParticles(const wchar_t *file, std::vector<ParticleEffect> &effects, std::vector<ParticleDefinition> &definitions)
	{
		FILE *f;

		if (_wfopen_s(&f, file, L"rb") != 0) {
			return false;
		}

		size_t size;
		fread(&size, sizeof(size_t), 1, f);

		for (int i = 0; i < size; ++i) {
			ParticleDefinition e;
			fread(&e, sizeof(ParticleDefinition), 1, f);

			definitions.push_back(e);
		}


		fread(&size, sizeof(size_t), 1, f);

		for (int i = 0; i < size; ++i) {
			ParticleEffect e;
			fread(&e, sizeof(ParticleEffect), 1, f);

			effects.push_back(e);
		}

		fclose(f);

		return true;
	}

}