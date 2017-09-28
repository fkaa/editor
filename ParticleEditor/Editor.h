#pragma once

namespace Editor {

struct Settings {
	float CameraDistance = 5.f;
	float CameraHeight = 15.f;
	float CameraSpeed = 0.075f;

	float ParticleSpeed = 1.0f;
	bool ParticlePaused = false;
	bool ParticleLoop = false;
};

void Init();
void Update(float dt);
void Render(float dt);

}