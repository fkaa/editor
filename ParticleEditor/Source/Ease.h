#pragma once

#include <DirectXMath.h>
#include <math.h>

using namespace DirectX;

#define EASE_FUNC(type, name, factor) inline type name(type start, type end, float t) \
{ \
	return Lerp(start, end, factor(t)); \
}

namespace ease {

inline float Lerp(float start, float end, float t)
{
	return start + t * (end - start);
}

inline XMVECTOR Lerp(XMVECTOR start, XMVECTOR end, float t)
{
	return XMVectorLerp(start, end, t);
}

inline float EaseInFactor(float t)
{
	return powf(t, 5);
}

inline float EaseOutFactor(float t)
{
	return 1 - powf(1 - t, 5);
}

EASE_FUNC(float, EaseIn, EaseInFactor)
EASE_FUNC(XMVECTOR, EaseIn, EaseInFactor)

EASE_FUNC(float, EaseOut, EaseOutFactor)
EASE_FUNC(XMVECTOR, EaseOut, EaseOutFactor)

}

enum class ParticleEase : uint32_t {
	Linear = 0,
	EaseIn,
	EaseOut,
	None
};

typedef float(*EaseFunc)(float, float, float);
typedef XMVECTOR(*EaseFuncV)(XMVECTOR, XMVECTOR, float);

extern EaseFunc ease_funcs[4];
extern EaseFuncV ease_funcs_xmv[4];

inline EaseFunc GetEaseFunc(ParticleEase ease)
{
	return ease_funcs[(int)ease];
}

inline EaseFuncV GetEaseFuncV(ParticleEase ease)
{
	return ease_funcs_xmv[(int)ease];
}